//----------------------------------------------------------------------------
// uix_components_complex.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "characterclass.h"
#include "uix.h"
#include "uix_graphic.h"
#include "uix_priv.h"
#include "uix_components_complex.h"
#include "uix_components_hellgate.h"
#include "uix_graphic.h"
#include "uix_scheduler.h"
#include "dictionary.h"
#include "markup.h"
#include "stringtable.h"
#include "excel.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "unit_priv.h"
#include "inventory.h"
#include "items.h"
#include "skills.h"
#include "c_ui.h"
#include "c_font.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_message.h"
#include "c_input.h"
#include "unittag.h"
#include "states.h"
#include "player.h"
#include "waypoint.h"
#include "fontcolor.h"
#include "quests.h"
#include "e_settings.h"
#include "c_quests.h"
#include "recipe.h"
#include "c_recipe.h"
#include "teams.h"
#include "skill_strings.h"
#include "waypoint.h"
#include "Economy.h"
#include "Currency.h"
#include "Weaponconfig.h"
#include "gameglobals.h"
#include "uix_money.h"
#include "language.h"
#include "quest_tutorial.h"
#include "pets.h"
#include "crafting_properties.h"
#include "stringreplacement.h"
#include "objects.h"

//MYTHOS
#include "levelareas.h"
#include "c_GameMapsClient.h"
#include "c_LevelAreasClient.h"
#include "LevelZones.h"
#include "LevelAreasKnownIterator.h"
#include "LevelAreaLinker.h"

#if !ISVERSION(SERVER_VERSION)
#include "svrstd.h"
#include "UserChatCommunication.h"
#include "c_chatNetwork.h"
#include "movement.h"
#include "pvp.h"
#include "ctf.h"



//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------

static inline float sSmallSkillIconScale(void)
{
	if (AppIsTugboat())
	{
		return 0.75f;
	}
	else
	{
		return 0.5f;
	}
}

#define MAX_STATES_DISPLAY_ITEMS		32

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

DWORD pgdwGameTickStateEnds[MAX_STATES_DISPLAY_ITEMS];
int pgnTrackingStates[MAX_STATES_DISPLAY_ITEMS];

//----------------------------------------------------------------------------
// UI Component methods
//----------------------------------------------------------------------------

UI_INVGRID::UI_INVGRID(void)
: UI_COMPONENT()
, m_pItemBkgrdFrame(NULL)
, m_pUseableBorderFrame(NULL)
, m_pUnidentifiedIconFrame(NULL)
, m_pBadClassIconFrame(NULL)
, m_pBadClassUnIDIconFrame(NULL)
, m_pLowStatsIconFrame(NULL)
, m_pSocketFrame(NULL)
, m_pModFrame(NULL)
, m_pRecipeCompleteFrame(NULL)
, m_pRecipeIncompleteFrame(NULL)
, m_dwModWontFitColor(0)
, m_dwModWontGoInItemColor(0)
, m_dwInactiveItemColor(0)
, m_fCellWidth(0.0f)
, m_fCellHeight(0.0f)
, m_fCellBorder(0.0f)
, m_nInvLocation(0)
, m_nInvLocDefault(0)
, m_nInvLocPlayer(0)
, m_nInvLocCabalist(0)
, m_nInvLocHunter(0)
, m_nInvLocTemplar(0)
, m_nInvLocMerchant(0)
, m_nGridWidth(0)
, m_nGridHeight(0)
, m_pItemAdj(NULL)
, m_pItemAdjStart(NULL)
{
}

UI_INVSLOT::UI_INVSLOT(void)
: UI_COMPONENT()
, m_pLitFrame(NULL)
, m_pSocketFrame(NULL)
, m_pModFrame(NULL)
, m_pLowStatsIconFrame(NULL)
, m_pUseableBorderFrame(NULL)
, m_pBadClassIconFrame(NULL)
, m_pBadClassUnIDIconFrame(NULL)
, m_pUnidentifiedIconFrame(NULL)
, m_nInvLocation(0)
, m_nInvUnitId(0)
, m_bUseUnitId(FALSE)
, m_fSlotScale(0.0f)
, m_dwModWontFitColor(0)
, m_dwModWontGoInItemColor(0)
, m_fModelBorderX(0.0f)
, m_fModelBorderY(0.0f)
{
}

UI_SKILLSGRID::UI_SKILLSGRID(void)
: UI_COMPONENT()
, m_pBorderFrame(NULL)
, m_pBkgndFrame(NULL)
, m_pBkgndFramePassive(NULL)
, m_pSelectedFrame(NULL)
, m_pSkillupBorderFrame(NULL)
, m_pSkillupButtonFrame(NULL)
, m_pSkillupButtonDownFrame(NULL)
, m_pSkillShiftButtonFrame(NULL)
, m_pSkillShiftButtonDownFrame(NULL)
, m_fCellWidth(0.0f)
, m_fCellHeight(0.0f)
, m_fCellBorderX(0.0f)
, m_fCellBorderY(0.0f)
, m_fLevelTextOffset(0.0f)
, m_nCellsX(0)
, m_nCellsY(0)
, m_nInvLocation(0)
, m_nSkillTab(0)
, m_bLookupTabNum(FALSE)
, m_nLastSkillRequestID(0)
, m_pIconPanel(NULL)
, m_pHighlightPanel(NULL)
, m_fIconWidth(0)
, m_fIconHeight(0)
, m_nSkillupSkill(0)
, m_nShiftToggleOnSound(0)
, m_nShiftToggleOffSound(0)
, m_pSkillPoint(NULL)
, m_bIsHotBar(FALSE)
{
}

UI_CRAFTINGGRID::UI_CRAFTINGGRID(void)
: UI_COMPONENT()
, m_pBorderFrame(NULL)
, m_pBkgndFrame(NULL)
, m_pBkgndFramePassive(NULL)
, m_pSelectedFrame(NULL)
, m_pSkillupBorderFrame(NULL)
, m_pSkillupButtonFrame(NULL)
, m_pSkillupButtonDownFrame(NULL)
, m_pSkillShiftButtonFrame(NULL)
, m_pSkillShiftButtonDownFrame(NULL)
, m_fCellWidth(0.0f)
, m_fCellHeight(0.0f)
, m_fCellBorderX(0.0f)
, m_fCellBorderY(0.0f)
, m_fLevelTextOffset(0.0f)
, m_nCellsX(0)
, m_nCellsY(0)
, m_nInvLocation(0)
, m_nSkillTab(0)
, m_bLookupTabNum(FALSE)
, m_nLastSkillRequestID(0)
, m_pIconPanel(NULL)
, m_pHighlightPanel(NULL)
, m_fIconWidth(0.0f)
, m_fIconHeight(0.0f)
, m_nSkillupSkill(0)
, m_pSkillPoint(NULL)
{
}

UI_TRADESMANSHEET::UI_TRADESMANSHEET(void)
: UI_COMPONENT()
, m_pBkgndFrame(NULL)
, m_pCreateButtonFrame(NULL)
, m_fCellHeight(0.0f)
, m_nCellsY(0)
, m_nInvLocation(0)
{
}

UI_UNIT_NAME_DISPLAY::UI_UNIT_NAME_DISPLAY(void)
: UI_COMPONENT()
, m_idUnitNameUnderCursor(0)
, m_fUnitNameUnderCursorDistSq(0.0f)
, m_pItemBorderFrame(NULL)
, m_pPickupLine(NULL)
, m_pUnidentifiedIconFrame(NULL)
, m_pBadClassIconFrame(NULL)
, m_pBadClassUnIDIconFrame(NULL)
, m_pLowStatsIconFrame(NULL)
, m_pHardcoreFrame(NULL)
, m_pLeagueFrame(NULL)
, m_pEliteFrame(NULL)
, m_pPVPOnlyFrame(NULL)
, m_dwHardcoreFrameColor(0)
, m_dwLeagueFrameColor(0)
, m_dwEliteFrameColor(0)
, m_fPlayerInfoSpacing(0.0f)
, m_fPlayerBadgeSpacing(0.0f)
, m_nPlayerInfoFontsize(0)
, m_nDisplayArea(0)
, m_fMaxItemNameWidth(0.0f)
{
}

UI_ACTIVESLOT::UI_ACTIVESLOT(void)
: UI_COMPONENT()
, m_nSlotNumber(0)
, m_nKeyCommand(0)
, m_pLitFrame(NULL)
, m_pIconPanel(NULL)
, m_dwTickCooldownOver(0)
{
}

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
void UIPartyCreateEnableEdits(BOOL bEnable, BOOL bCreatePartyButton = -1);

//----------------------------------------------------------------------------
// statics and types for New Player Matching dialog
//----------------------------------------------------------------------------

enum PlayerMatchModes
{
	PMM_PARTY = 0,
	PMM_PLAYER,
	PMM_MATCH,

	PMM_PARTY_CREATE,
	PMM_LIST_SELF,
	PMM_MATCH_CREATE,
	PMM_AUTOMATCH,

	PMM_DEFAULT
};

static PlayerMatchModes s_CurrentPlayerMatchMode = PMM_PARTY;
static PlayerMatchModes s_LastTab = PMM_PARTY;
static UI_COMPONENT *	s_pCurPanel = NULL;
static BOOL				s_bGuildOnly = FALSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillCanGoInActiveBar(
	int nSkillID)
{
	if (nSkillID == INVALID_ID)
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( AppGetCltGame(), nSkillID );
	if (!pSkillData)
		return FALSE;

	if ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_HOTKEYABLE ) )	
	{
		return FALSE;
	}

	UNIT *pPlayer = UIGetControlUnit();
	ASSERT_RETFALSE(pPlayer);

	if (SkillGetLevel(pPlayer, nSkillID) <= 0)
		return FALSE;

	return TRUE;
}

static BOOL sUnitCanGoInActiveBar(
	UNITID idUnit)
{
	if (idUnit != INVALID_ID)
	{
		UNIT *pUnit = UnitGetById(AppGetCltGame(), idUnit);
		if (pUnit)
		{	
			if ( ItemIsQuestActiveBarOk( UnitGetContainer(pUnit), pUnit ) == USE_RESULT_OK )
			{
				return TRUE;
			}

			if ( AppIsTugboat() &&
				( UnitIsA( pUnit, UNITTYPE_ANCHORSTONE ) ||
				 UnitIsA( pUnit, UNITTYPE_TOWNPORTAL ) ) )
			{
				return TRUE;
			}

			if (!UnitIsA(pUnit, UNITTYPE_CONSUMABLE) )
			{
				return FALSE;
			}

			if (UnitIsA(pUnit, UNITTYPE_SINGLE_USE_RECIPE))
			{
				return FALSE;
			}

			if (ItemBelongsToAnyMerchant(pUnit))
			{
				return FALSE;
			}

			return TRUE;
		}
	}

	return FALSE;
}

UI_TEXTURE_FRAME *UIGetStdSkillIconBackground()
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_SKILLGRID1);
	if (!pComponent)
		return NULL;

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(pComponent);

	if (pGrid)
		return pGrid->m_pBkgndFrame;

	return NULL;
}

UIX_TEXTURE *UIGetStdSkillIconBackgroundTexture()
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_SKILLGRID1);
	if (!pComponent)
		return NULL;

	return UIComponentGetTexture(pComponent);
}

UI_TEXTURE_FRAME *UIGetStdSkillIconBorder()
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_SKILLGRID1);
	if (!pComponent)
		return NULL;

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(pComponent);

	if (pGrid)
		return pGrid->m_pBorderFrame;

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline UNIT_TAG_HOTKEY* UIUnitGetHotkeyTag(
	int slot)
{
	UNIT* unit = UIGetControlUnit();
	if (!unit)
	{
		return NULL;
	}
	return UnitGetHotkeyTag(unit, slot);
}

BOOL UIDrawSkillIcon(
	const UI_DRAWSKILLICON_PARAMS& tParams)
{
	ASSERT_RETFALSE(tParams.component);

	if (!tParams.pTexture)
	{
		return FALSE;
	}

	const SKILL_DATA* pSkillData = NULL;
	if (tParams.nSkill != INVALID_ID)
	{
		pSkillData = (SKILL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLS, tParams.nSkill);
	}

	UIX_TEXTURE *pSkillTexture = tParams.pSkillTexture;
	if (pSkillTexture == NULL &&
		pSkillData)
	{
		const SKILLTAB_DATA *pSkillTabData = pSkillData->nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pSkillData->nSkillTab) : NULL;
		if (pSkillTabData)
		{
			pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
		}
	}

	if (tParams.pSelectedFrame)
	{
		DWORD dwColor = UI_MAKE_COLOR(tParams.byAlpha, tParams.pSelectedFrame->m_dwDefaultColor);
		UIComponentAddElement(tParams.component, tParams.pTexture, tParams.pSelectedFrame, tParams.pos, dwColor, NULL); //&rect);
	}

	if (tParams.pBackgroundFrame)
	{
		UI_POSITION posBackground = tParams.pos;
		UI_RECT rectClipBackground = tParams.rect;
		if (tParams.pBackgroundComponent != tParams.component)
		{
			// make sure they're gonna line up
			UI_POSITION posDiff = UIComponentGetAbsoluteLogPosition(tParams.component) - UIComponentGetAbsoluteLogPosition(tParams.pBackgroundComponent);
			posBackground += posDiff;
			rectClipBackground.m_fX1 += posDiff.m_fX;
			rectClipBackground.m_fY1 += posDiff.m_fY;
			rectClipBackground.m_fX2 += posDiff.m_fX;
			rectClipBackground.m_fY2 += posDiff.m_fY;
		}
		if( AppIsHellgate() )
		{
			DWORD dwColor = GFXCOLOR_WHITE; //UI_MAKE_COLOR(tParams.byAlpha, (!tParams.bGreyout && pSkillData ? GetFontColor(pSkillData->nIconBkgndColor) : GFXCOLOR_VDKGRAY));
			UIComponentAddElement(tParams.pBackgroundComponent, tParams.pTexture, tParams.pBackgroundFrame, posBackground, dwColor, &rectClipBackground);
		}
		else if( AppIsTugboat() )
		{
			DWORD dwColor = UI_MAKE_COLOR(tParams.byAlpha, GFXCOLOR_WHITE);
			UIComponentAddElement(tParams.pBackgroundComponent, tParams.pTexture, tParams.pBackgroundFrame, posBackground, dwColor, NULL);
		}
	}

	if (!pSkillData || !pSkillTexture)
	{
		return FALSE;
	}

	UI_TEXTURE_FRAME* pSkillIcon = (UI_TEXTURE_FRAME*)StrDictionaryFind(pSkillTexture->m_pFrames, pSkillData->szSmallIcon);
	if (pSkillIcon)
	{
		UI_POSITION skillpos = tParams.pos;
		if (tParams.bNegateOffsets)
		{
			skillpos.m_fX += pSkillIcon->m_fXOffset;
			skillpos.m_fY += pSkillIcon->m_fYOffset;
		}

		DWORD dwColor = GetFontColor(pSkillData->nIconColor);
		if (tParams.bGreyout)
		{
			dwColor = GFXCOLOR_DKGRAY;
		}
		if (tParams.bTooCostly)
			dwColor = AppIsHellgate() ? GFXCOLOR_RED : GFXCOLOR_DKGRAY;
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_DISABLED ) )
			dwColor = AppIsHellgate() ? GFXCOLOR_RED : GFXCOLOR_DKGRAY;
		dwColor = UI_MAKE_COLOR(tParams.byAlpha, dwColor);
		// Tugboat auto-centers smaller skill icons in their background frame
		if( AppIsTugboat() )
		{
			BOOL bIconIsRed;
			float fScale;
			int nTicksRemaining;
			c_SkillGetIconInfo( UIGetControlUnit(), tParams.nSkill, bIconIsRed, fScale, nTicksRemaining, TRUE );

			UI_SIZE sizeIcon;
			sizeIcon.m_fWidth = (tParams.fSkillIconWidth != -1.0f ? tParams.fSkillIconWidth : pSkillIcon->m_fWidth);
			sizeIcon.m_fHeight = (tParams.fSkillIconHeight != -1.0f ? tParams.fSkillIconHeight : pSkillIcon->m_fHeight);

			UI_GFXELEMENT* pElement = UIComponentAddElement(tParams.component, pSkillTexture, pSkillIcon, skillpos, dwColor, &tParams.rect, FALSE, tParams.fScaleX, tParams.fScaleY, &sizeIcon);
			if( pElement )
			{
				pElement->m_qwData = tParams.nSkill;
				pElement->m_fZDelta = -.0001f;
			}
			if (fScale != 1)
			{
				UI_RECT ClipRect = tParams.rect;
				//ClipRect.m_fX1 *= UIGetScreenToLogRatioX( tParams.component->m_bNoScale );
				//ClipRect.m_fX2 *= UIGetScreenToLogRatioX( tParams.component->m_bNoScale );
				//ClipRect.m_fY1 *= UIGetScreenToLogRatioY( tParams.component->m_bNoScale );
				//ClipRect.m_fY2 *= UIGetScreenToLogRatioY( tParams.component->m_bNoScale );
				float X = ClipRect.m_fX1;
				float Y = ClipRect.m_fY1;
				float Width = tParams.fSkillIconWidth;// * UIGetScreenToLogRatioX( tParams.component->m_bNoScale );
				float Height = tParams.fSkillIconHeight;// * UIGetScreenToLogRatioY( tParams.component->m_bNoScale );
				pElement = UIComponentAddBoxElement(tParams.component, X, Y + ( Height) * fScale , X + Width , Y + ( Height  ), NULL, UI_HILITE_GRAY, 180, &ClipRect);
				if( pElement )
				{
					pElement->m_qwData = tParams.nSkill;
					pElement->m_fZDelta = -.0002f;
				}
				//WCHAR buf[32];
				//PIntToStr(buf, 32, (nTicksRemaining + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND);
				//UIComponentAddTextElement(tParams.component, NULL, UIComponentGetFont(tParams.component), UIComponentGetFontSize(tParams.component), buf, UI_POSITION(), GFXCOLOR_WHITE, NULL, UIALIGN_BOTTOM);
			}

		}
		else if( AppIsHellgate() )
		{
			UI_SIZE sizeIcon;
			sizeIcon.m_fWidth = (tParams.fSkillIconWidth != -1.0f ? tParams.fSkillIconWidth : pSkillIcon->m_fWidth);
			sizeIcon.m_fHeight = (tParams.fSkillIconHeight != -1.0f ? tParams.fSkillIconHeight : pSkillIcon->m_fHeight);
			UI_GFXELEMENT* pElement = UIComponentAddElement(tParams.component, pSkillTexture, pSkillIcon, skillpos, dwColor, &tParams.rect, FALSE, tParams.fScaleX, tParams.fScaleY, &sizeIcon);
			pElement->m_qwData = tParams.nSkill;
		}
	}
	else
	{
		// Add some tiny text to ID the skill
		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(tParams.component);
		int nFontSize = UIComponentGetFontSize(tParams.component);
		if (!pFont)
		{
			pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
			nFontSize = UIComponentGetFontSize(UIComponentGetByEnum(UICOMP_SKILLGRID1));
		}
		UI_POSITION pos(tParams.rect.m_fX1, tParams.rect.m_fY1);
		UI_SIZE size(tParams.rect.m_fX2 - tParams.rect.m_fX1, tParams.rect.m_fY2 - tParams.rect.m_fY1);

		DWORD dwColor = UI_MAKE_COLOR(tParams.byAlpha, GFXCOLOR_WHITE);

		WCHAR szName[256];
		PStrCopy(szName, StringTableGetStringByIndex(pSkillData->nDisplayName), 256);
		DoWordWrap(szName, size.m_fWidth, pFont, tParams.component->m_bNoScaleFont, nFontSize);
		UIComponentAddTextElement(tParams.component, tParams.pTexture, pFont, nFontSize, szName, pos, dwColor, &tParams.rect, UIALIGN_CENTER, &size );
	}

	if (AppIsTugboat() && tParams.bShowHotkey)
    {
		BOOL bIsHotBar = FALSE;
		int bLeftAssignment = FALSE;
		if( tParams.component->m_pParent &&
			UIComponentIsSkillsGrid( tParams.component->m_pParent ) )
		{
			bIsHotBar = UICastToSkillsGrid( tParams.component->m_pParent )->m_bIsHotBar;
			if( bIsHotBar )
			{
				bLeftAssignment = (int)tParams.component->m_pParent->m_dwParam == 0;
			}
		}
	    // find out if hotkeys are assigned, and add txet if so.
	    for( int t = TAG_SELECTOR_SPELLHOTKEY1; t <= TAG_SELECTOR_SPELLHOTKEY12; t++ )
	    {
		    UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(t);
			int Tag = bLeftAssignment ? 0 : 1;

		    if( tag && tag->m_nSkillID[Tag] != INVALID_ID )
		    {
			    
			    if( tag->m_nSkillID[Tag] == tParams.nSkill )
			    {
				    // Add some tiny text to ID the skill
				    UIX_TEXTURE_FONT *pFont = UIComponentGetFont(tParams.component);
				    if (!pFont)
				    {
					    pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
				    }
				    UI_POSITION pos(tParams.rect.m_fX1 , tParams.rect.m_fY1 );
				    UI_SIZE size(tParams.fSkillIconWidth, tParams.fSkillIconHeight);

				    DWORD dwColor = UI_MAKE_COLOR(tParams.byAlpha, GFXCOLOR_WHITE);
    
				    WCHAR szName[256];
				    PStrPrintf(szName, 5, L"F%d", t - TAG_SELECTOR_SPELLHOTKEY1 +1);
				    UI_GFXELEMENT *pElement = UIComponentAddTextElement(tParams.component, tParams.pTexture, pFont, UIComponentGetFontSize(tParams.component), szName, pos, dwColor, &tParams.rect, UIALIGN_TOPRIGHT, &size );
					if( pElement )
					{
						pElement->m_fZDelta = -.001f;
					}
				    break;
			    }
		    }
	    }
    }

	if (tParams.pBorderFrame)
	{
		DWORD dwColor = UI_MAKE_COLOR(tParams.byAlpha, tParams.bGreyout ? GFXCOLOR_DKGRAY : GFXCOLOR_WHITE);
		UIComponentAddElement(tParams.component, tParams.pTexture, tParams.pBorderFrame, tParams.pos, dwColor, &tParams.rect);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIncItemAdjust(
	UI_INV_ITEM_GFX_ADJUSTMENT& ItemAdj,
	UI_INV_ITEM_GFX_ADJUSTMENT& ItemAdjStart,
	const TIME tiCurrentTime,
	const UI_ANIM_INFO & tAnimInfo)
{
	BOOL bDoAgain = FALSE;

	DWORD dwElapsed = TimeGetElapsed(tAnimInfo.m_tiAnimStart, tiCurrentTime);
	int nRemaining = tAnimInfo.m_dwAnimDuration - dwElapsed;
	float fPercent = nRemaining > 0 ? (float)nRemaining / (float)tAnimInfo.m_dwAnimDuration : 0.0f;

	if (ItemAdj.fXOffset != 0.0f)
	{
		ItemAdj.fXOffset = ItemAdjStart.fXOffset * fPercent;
		if (fabs(ItemAdj.fXOffset) < 1.0f) ItemAdj.fXOffset = 0.0f;
		if (ItemAdj.fXOffset != 0.0f)
			bDoAgain = TRUE;
	}
	if (ItemAdj.fYOffset != 0.0f)
	{
		ItemAdj.fYOffset = ItemAdjStart.fYOffset * fPercent;
		if (fabs(ItemAdj.fYOffset) < 1.0f) ItemAdj.fYOffset = 0.0f;
		if (ItemAdj.fYOffset != 0.0f)
			bDoAgain = TRUE;
	}

	if (ItemAdj.fScale > 1.0f)
	{
		ItemAdj.fScale = ((ItemAdjStart.fScale - 1.0f) * fPercent) + 1.0f;
	}
	if (ItemAdj.fScale < 1.0f)
	{
		ItemAdj.fScale = 1.0f;
	}
	if (ItemAdj.fScale != 1.0f)
	{
		bDoAgain = TRUE;
	}

	return bDoAgain;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIInvGridDeflate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_INVGRID *invgrid = (UI_INVGRID *)data.m_Data1;
	ASSERT_RETURN(invgrid);

	BOOL bFromPaint = (BOOL)data.m_Data2;

	if (invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket);
	}

	BOOL bDoAgain = FALSE;

	TIME tiCurrentTime = AppCommonGetCurTime();
	for (int i=0; i < invgrid->m_nGridWidth * invgrid->m_nGridHeight; i++)
	{
		if (invgrid->m_pItemAdj[i].fScale != 1.0f ||
			invgrid->m_pItemAdj[i].fXOffset != 0.0f ||
			invgrid->m_pItemAdj[i].fYOffset != 0.0f)
		{
			bDoAgain |= sIncItemAdjust(invgrid->m_pItemAdj[i], invgrid->m_pItemAdjStart[i], tiCurrentTime, invgrid->m_tItemAdjAnimInfo);
		}
	}

	if (!bFromPaint)
	{
		UIComponentHandleUIMessage(invgrid, UIMSG_PAINT, 0, 0);
	}

	if (bDoAgain)
	{
		// schedule the next one
		invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIInvGridDeflate, CEVENT_DATA((DWORD_PTR)invgrid, 0));	
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIInvGetItemGridBackgroundColor(
	UNIT * item)
{
	ASSERT_RETINVALID(item);

	if (UIGetCursorState() == UICURSOR_STATE_SKILL_PICK_ITEM)
	{
		return INVALID_ID;
	}

	UNIT * control = GameGetControlUnit(AppGetCltGame());
	if (control)
	{
		UNIT *pContainer = UnitGetContainer(item);
		if (UnitGetUltimateContainer(item) == control &&
			pContainer != control)
		{
			// if this item is on the player, but is within another item, check against the container
			control = pContainer;
		}
		if (ItemIsInEquipLocation(control, item) && !ItemIsEquipped(item, control))
		{
			return FONTCOLOR_UNUSABLE_ITEM_BORDER;
		}
	}

	int nQuality = ItemGetQuality( item );
	if (nQuality != INVALID_LINK)
	{
		const ITEM_QUALITY_DATA * quality_data = ItemQualityGetData(nQuality);
		ASSERT_RETVAL(quality_data, INVALID_ID);
		if (quality_data->nGridColor != INVALID_ID)
		{
			return quality_data->nGridColor;
		}
	}
	
	int color = INVALID_ID;
	int dom = -1;
	STATS * modlist = NULL;
	STATS * nextlist = StatsGetModList(item, SELECTOR_AFFIX);
	while ((modlist = nextlist) != NULL)
	{
		nextlist = StatsGetModNext(modlist);

		int affix = StatsGetStat(modlist, STATS_AFFIX_ID);
		const AFFIX_DATA * affixdata = AffixGetData(affix);
		ASSERT_CONTINUE(affixdata);
		if (affixdata->nGridColor != INVALID_ID && affixdata->nDominance > dom)
		{
			color = affixdata->nGridColor;
			dom = affixdata->nDominance;
		}
	}
	if (color != INVALID_ID)
	{
		return color;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIInvGetItemGridBorderColor(
	UNIT * item,
	UI_COMPONENT *pComponent,
	UI_TEXTURE_FRAME **ppFrame,
	int *pnFrameColor)
{
	ASSERT_RETZERO(pComponent);

	if (ppFrame)
		*ppFrame = NULL;
	if (pnFrameColor)
		*pnFrameColor = FONTCOLOR_UNUSABLE_ITEM_BORDER;
	
	//UNIT * cursor = UnitGetById(AppGetCltGame(), UIGetCursorUnit());
	//if (cursor && UnitIsA(cursor, UNITTYPE_MOD))
	//{
	//	return INVALID_ID;
	//}

	if (UIGetCursorState() == UICURSOR_STATE_SKILL_PICK_ITEM)
	{
		return INVALID_ID;
	}

	UI_INVGRID *pInvGrid = NULL;
	UI_INVSLOT *pInvSlot = NULL;
	UI_UNIT_NAME_DISPLAY *pUnitDisplay = NULL;
	if (ppFrame)
	{
		if (UIComponentIsInvGrid(pComponent))
		{
			pInvGrid = UICastToInvGrid(pComponent);
		}
		if (UIComponentIsInvSlot(pComponent))
		{
			pInvSlot = UICastToInvSlot(pComponent);
		}
		if (UIComponentIsUnitNameDisplay(pComponent))
		{
			pUnitDisplay = UICastToUnitNameDisplay(pComponent);
		}

	}

	// we need to ignore any equipped item that this item would replace
	UNIT *pUnit = NULL;
	
	if (AppIsHellgate())
	{
		UI_COMPONENT *pPaperdoll = UIComponentGetByEnum(UICOMP_PAPERDOLL);
		if (pPaperdoll && UIComponentGetActive(pPaperdoll))
		{
			pUnit = UIComponentGetFocusUnit(pPaperdoll);
		}
		else
		{
			pUnit = UIComponentGetFocusUnit(pComponent);
		}

		if (pInvSlot &&
			pInvSlot->m_nInvLocation == INVALID_LINK)
		{
			pUnit = NULL;
		}
	}

	if (!pUnit)
		pUnit = GameGetControlUnit(AppGetCltGame());

	BOOL bUnidentified = ItemIsUnidentified(item);
	const UNIT_DATA* pItemData = ItemGetData(item);			

	if (!InventoryItemMeetsClassReqs(item, pUnit) ||
		(pItemData && UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY ) && UnitIsA(pUnit, UNITTYPE_PLAYER) && !PlayerIsSubscriber(pUnit)) )
	{
		if (bUnidentified)
		{
			if (pInvGrid)
				*ppFrame = pInvGrid->m_pBadClassUnIDIconFrame;
			if (pInvSlot)
				*ppFrame = pInvSlot->m_pBadClassUnIDIconFrame;
			if (pUnitDisplay)
				*ppFrame = pUnitDisplay->m_pBadClassUnIDIconFrame;
		}
		else
		{
			if (pInvGrid)
				*ppFrame = pInvGrid->m_pBadClassIconFrame;
			if (pInvSlot)
				*ppFrame = pInvSlot->m_pBadClassIconFrame;
			if (pUnitDisplay)
				*ppFrame = pUnitDisplay->m_pBadClassIconFrame;
		}
		return FONTCOLOR_UNUSABLE_ITEM_BORDER;
	}

	if (bUnidentified)
	{
		if (pInvGrid)
			*ppFrame = pInvGrid->m_pUnidentifiedIconFrame;
		if (pInvSlot)
			*ppFrame = pInvSlot->m_pUnidentifiedIconFrame;
		if (pUnitDisplay)
			*ppFrame = pUnitDisplay->m_pUnidentifiedIconFrame;
		return INVALID_ID;
	}

	if (UnitIsA(item, UNITTYPE_SINGLE_USE_RECIPE))
	{
		if (pInvGrid)
		{
			int nRecipe = UnitGetStat(item, STATS_RECIPE_SINGLE_USE );
			ASSERTX_RETINVALID( nRecipe != INVALID_LINK, "Expected single use recipe link" );
			UNIT *pPlayer = UIGetControlUnit();
			UNITID idRecipeGiver = UnitGetId( item );
			if (RecipeIsComplete( pPlayer, nRecipe, 0, idRecipeGiver, TRUE ))
			{
				*ppFrame = pInvGrid->m_pRecipeCompleteFrame;
				*pnFrameColor = FONTCOLOR_SINGLE_USE_RECIPE_COMPLETE;
			}
			else
			{
				//*ppFrame = pInvGrid->m_pRecipeIncompleteFrame;
				//*pnFrameColor = FONTCOLOR_SINGLE_USE_RECIPE_INCOMPLETE;
			}
		}
		return INVALID_ID;
	}
	
	if (!ItemMeetsStatReqsForUI(item, INVALID_LINK, pUnit))
	{
		if (pInvGrid)
			*ppFrame = pInvGrid->m_pLowStatsIconFrame;
		if (pUnitDisplay)
			*ppFrame = pUnitDisplay->m_pLowStatsIconFrame;
		if (pInvSlot)
			*ppFrame = pInvSlot->m_pLowStatsIconFrame;

		return INVALID_ID;
	}

	if (!InventoryItemHasUseableQuality(item))
	{
		return FONTCOLOR_UNUSABLE_ITEM_BORDER;
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_SIZE sGetItemSizeInGrid(
	UNIT *pItem,
	UI_INVGRID *pInvGrid,
	INVLOC_HEADER *pInvLocInfo)
{
	int nItemInvHeight = 1;
	int nItemInvWidth = 1;
	if (!InvLocTestFlag(pInvLocInfo, INVLOCFLAG_ONESIZE))
	{
		nItemInvHeight = MAX(1, UnitGetStat(pItem, STATS_INVENTORY_HEIGHT));
		nItemInvWidth = MAX(1, UnitGetStat(pItem, STATS_INVENTORY_WIDTH));
	}
	UI_SIZE tItemSizeInGrid((pInvGrid->m_fCellWidth * nItemInvWidth) + (pInvGrid->m_fCellBorder * (nItemInvWidth - 1)), 
							(pInvGrid->m_fCellHeight * nItemInvHeight) + (pInvGrid->m_fCellBorder * (nItemInvHeight - 1)));

	return tItemSizeInGrid;
}

BOOL UIInvGridGetItemSize(
	UI_INVGRID * pInvGrid,
	UNIT * pItem, 
	UI_SIZE &tSize)
{
	UNIT* container = UIComponentGetFocusUnit(pInvGrid);
	if (!container)
	{
		return FALSE;
	}

	INVLOC_HEADER tInvlocInfo;
	if (!UnitInventoryGetLocInfo(container, pInvGrid->m_nInvLocation, &tInvlocInfo))
	{
		return FALSE;
	}

	tSize = sGetItemSizeInGrid(pItem, pInvGrid, &tInvlocInfo);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sUISocketedItemOnLButtonDownWithCursorUnit( UNIT* container )
{
	UNITID idCursorUnit = UIGetCursorUnit();
	UNIT* unitCursor = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!unitCursor)
	{

		UISetCursorUnit(INVALID_ID, TRUE);
		return FALSE;
	}

	if (ItemBelongsToAnyMerchant(container) || 
		ItemBelongsToAnyMerchant(unitCursor))
	{
		return FALSE;
	}

	if( ItemIsACraftedItem( container ) && !CRAFTING::CraftingCanModItem( container, unitCursor ) )
	{
		return FALSE;	
	}


	float x, y;
	UIGetCursorPosition(&x, &y);

	BOOL bCanReplace = ItemIsACraftedItem( container ) && UnitIsA( unitCursor, UNITTYPE_CRAFTING_MOD );


	PARAM dwParam = 0;
	int Location = INVALID_ID;
	int GridIdx = INVALID_ID;
	BOOL Done = FALSE;
	while ( !Done )
	{
		STATS_ENTRY stats_entry[16];
		int count = UnitGetStatsRange(container, STATS_ITEM_SLOTS, dwParam, stats_entry, 16);
		if (count <= 0)
		{
			break;
		}
		for (int ii = 0; ii < count; ii++)
		{
			int location = stats_entry[ii].GetParam();

			int slots = stats_entry[ii].value;
			if (slots <= 0)
			{
				continue;
			}

			for (int jj = 0; jj < slots; jj++)
			{

				UNIT* item = UnitInventoryGetByLocationAndIndex(container, location, jj );
				// a free slot!
				if( !item || bCanReplace)
				{
					Location = location;
					GridIdx = jj;
					Done = TRUE;
					break;
				}
			}
		}
		if (count < 64)
		{
			break;
		}
		dwParam = stats_entry[count-1].GetParam() + 1;
	}

	
	if( Location != INVALID_ID &&
		GridIdx != INVALID_ID )
	{
		if ( ( bCanReplace && UnitInventoryTestIgnoreExisting(container, unitCursor, Location, GridIdx) ) ||
			 UnitInventoryTest(container, unitCursor, Location, GridIdx) )
		{
			MSG_CCMD_ITEMINVPUT msg;
			msg.idContainer = UnitGetId(container);
			msg.idItem = idCursorUnit;
			msg.bLocation = (BYTE)Location;
			msg.bX = (BYTE)GridIdx;
			msg.bY = 0;
			c_SendMessage(CCMD_ITEMINVPUT, &msg);
			UnitWaitingForInventoryMove(unitCursor, TRUE);
			UIPlayItemPutdownSound(unitCursor, container, Location);
			//UISetCursorUnit(INVALID_ID, TRUE);
			return TRUE;
		}
	}


	return FALSE;
}

//draws gems and mods (Heraldries/Heraldry/gems )
inline void sUIX_DrawGemsAndMods( UI_INVGRID* invgrid,
								  UI_INVSLOT* invslot,								  
								  UNIT *item )
{

	if( AppIsHellgate() ||
		!(invgrid || invslot ) )		//only tugboat
		return;	
	
	UIX_TEXTURE* texture( NULL );		
	UNIT* pActualMerchant = TradeGetTradingWith( UIGetControlUnit() );
	
	

	UNIT* pCursorItem = UnitGetById(AppGetCltGame(), UIGetCursorUnit());

	if (!( pActualMerchant && ItemBelongsToAnyMerchant( item ) && UnitIsGambler( pActualMerchant ) ) &&
		( ( pCursorItem && UnitIsA(pCursorItem, UNITTYPE_MOD) ) || UnitGetId( item ) == UIGetHoverUnit() ) )	// in tug, we show the slots and what's in them in the actual inventory
	{
		UI_RECT rectComponent;
		UI_GFXELEMENT* element = NULL;
		int nSlots = ItemGetModSlots( item );
		int nModSlots = CRAFTING::CraftingGetNumOfCraftingSlots( item );
		REF( nModSlots );
		UI_SIZE sizeIcon;
		float YStep, X, Y, XIcon, YIcon, gx, gy;
		BOOL bItemIsACraftedItem = ItemIsACraftedItem( item );
		UI_COMPONENT* pLitFrameComponent( NULL );
		UI_TEXTURE_FRAME *m_pSocketFrame( NULL );
		UI_TEXTURE_FRAME *m_pModFrame( NULL );

		if( bItemIsACraftedItem && pCursorItem && !UnitIsA( pCursorItem, UNITTYPE_CRAFTING_MOD ) )
		{
			return;
		}
		if( !bItemIsACraftedItem && pCursorItem && UnitIsA( pCursorItem, UNITTYPE_CRAFTING_MOD ) )
		{
			return;
		}
		if( invgrid )
		{
			int x, y;
			UnitGetInventoryLocation(item, NULL, &x, &y);
			gx = (float)x * (invgrid->m_fCellWidth + invgrid->m_fCellBorder);
			gy = (float)y * (invgrid->m_fCellHeight + invgrid->m_fCellBorder);
			m_pSocketFrame = invgrid->m_pSocketFrame;
			m_pModFrame = invgrid->m_pModFrame;
			pLitFrameComponent = UIComponentFindChildByName(invgrid, "sublitframe");	// for Tugboat
			rectComponent = UIComponentGetRectBase( invgrid );
			texture = UIComponentGetTexture(invgrid);
			YStep = invgrid->m_fCellHeight;
			X = invgrid->m_fCellWidth * item->pUnitData->nInvWidth / 2 - ( YStep * ( bItemIsACraftedItem ? 1.5f : 1.0f ) ) * .5f;
			Y = invgrid->m_fCellHeight * item->pUnitData->nInvHeight / 2 - YStep * (float)( nSlots ) * .5f;			
			XIcon = invgrid->m_fCellWidth * item->pUnitData->nInvWidth / 2 - YStep * .5f;
			YIcon = invgrid->m_fCellHeight * item->pUnitData->nInvHeight / 2 - YStep * (float)( nSlots ) * .5f;			
			sizeIcon.m_fWidth = invgrid->m_fCellWidth * ( bItemIsACraftedItem  ? 1.5f : 1.0f );
			sizeIcon.m_fHeight = invgrid->m_fCellHeight;
		}
		else
		{
			gx = 0;
			gy = 0;
			m_pSocketFrame = invslot->m_pSocketFrame;
			m_pModFrame = invslot->m_pModFrame;
			pLitFrameComponent = UIComponentFindChildByName(invslot, "sublitframe");	// for Tugboat
			rectComponent = UIComponentGetRectBase( invslot );			
			texture = UIComponentGetTexture(invslot);
			sizeIcon.m_fWidth = ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale * ( bItemIsACraftedItem ? 1.5f : 1.0f );
			sizeIcon.m_fHeight = ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale;
			YStep = ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale;
			X = invslot->m_fWidth * .5f -  ( YStep * ( bItemIsACraftedItem ? 1.5f : 1.0f ) ) * .5f;
			Y = invslot->m_fHeight * .5f - YStep * (float)( nSlots  ) * .5f;
			XIcon = invslot->m_fWidth * .5f -  YStep * .5f;
			YIcon = invslot->m_fHeight * .5f - YStep * (float)( nSlots  ) * .5f;
		}

		ASSERT_RETURN( pLitFrameComponent );

		
		if( bItemIsACraftedItem )
		{
			BOOL bIsRed = ( !pCursorItem || ( pCursorItem && CRAFTING::CraftingCanModItem( item, pCursorItem ) ) ) ? FALSE : TRUE;
			for( int i = 0; i < nSlots; i++ )
				UIComponentAddElement( pLitFrameComponent, texture, m_pModFrame, UI_POSITION(gx + X, gy + Y + YStep * (FLOAT)i), bIsRed ? GFXCOLOR_RED : GFXCOLOR_WHITE, &rectComponent, FALSE, 1.0f, 1.0f, &sizeIcon );
		}
		else
		{
			for( int i = 0; i < nSlots; i++ )
				UIComponentAddElement( pLitFrameComponent, texture, m_pSocketFrame, UI_POSITION(gx + X, gy + Y + YStep * (FLOAT)i), GFXCOLOR_WHITE, &rectComponent, FALSE, 1.0f, 1.0f, &sizeIcon );
		}

		if( nSlots > 0 )
		{

			PARAM dwParam = 0;
			while (TRUE)
			{
				STATS_ENTRY stats_entry[16];
				int count = UnitGetStatsRange(item, STATS_ITEM_SLOTS, dwParam, stats_entry, 16);
				if (count <= 0)
				{
					break;
				}
				for (int ii = 0; ii < count; ii++)
				{
					int location = stats_entry[ii].GetParam();
					int slots = stats_entry[ii].value;
					if (slots <= 0)
					{
						continue;
					}

					for (int jj = 0; jj < slots; jj++)
					{
						UNIT* socketed = UnitInventoryGetByLocationAndIndex(item, location, jj );
						// a socketed item!
						if( socketed )
						{
							if( socketed->pIconTexture )
							{
								BOOL bLoaded = UITextureLoadFinalize( socketed->pIconTexture );
								REF( bLoaded );
								UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(socketed->pIconTexture->m_pFrames, "icon");
								if (frame && bLoaded)
								{
									UI_SIZE sizeIcon;
									if( invgrid )
									{
										UI_RECT gridcellrect;
										gridcellrect.m_fX1 = gx;
										gridcellrect.m_fY1 = gy;
										gridcellrect.m_fX2 = gx + invgrid->m_fCellWidth * item->pUnitData->nInvWidth;
										gridcellrect.m_fY2 = gy + invgrid->m_fCellHeight * item->pUnitData->nInvHeight;
										sizeIcon.m_fWidth = invgrid->m_fCellWidth;
										sizeIcon.m_fHeight = invgrid->m_fCellHeight;
										element = UIComponentAddElement( pLitFrameComponent, socketed->pIconTexture, frame, UI_POSITION(gx + XIcon, gy + YIcon + YStep * (FLOAT)jj) , GFXCOLOR_WHITE , &gridcellrect, 0, 1, 1, &sizeIcon  );
									}
									else
									{
										sizeIcon.m_fWidth = ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale;
										sizeIcon.m_fHeight = ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale;
										element = UIComponentAddElement( pLitFrameComponent, socketed->pIconTexture, frame, UI_POSITION( XIcon, YIcon + YStep * (FLOAT)jj ) , GFXCOLOR_WHITE , &UI_RECT(0.0f, 0.0f, invslot->m_fWidth, invslot->m_fHeight), 0, 1.0f, 1.0f, &sizeIcon  );
									}
								}
								else if( invgrid )
								{
									// it's not asynch loaded yet - schedule a repaint so we can take care of it
									if (invgrid->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
									{
										CSchedulerCancelEvent(invgrid->m_tMainAnimInfo.m_dwAnimTicket);
									}
									invgrid->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR)invgrid, UIMSG_PAINT, 0, 0));

								}

							}					
						}
					}
				}
				if (count < 64)
				{
					break;
				}
				dwParam = stats_entry[count-1].GetParam() + 1;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_INVGRID* invgrid = UICastToInvGrid(component);

	UIComponentRemoveAllElements(invgrid);

	if (AppGetState() != APP_STATE_IN_GAME)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if( UIComponentGetVisible( component ) == FALSE )
	{
		return UIMSG_RET_NOT_HANDLED;
	}


	UNIT* container = UIComponentGetFocusUnit(invgrid);
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	INVLOC_HEADER tInvlocInfo;
	if (!UnitInventoryGetLocInfo(container, invgrid->m_nInvLocation, &tInvlocInfo))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* pActualMerchant = TradeGetTradingWith( UIGetControlUnit() );
	UI_COMPONENT* pLitFrameComponent = UIComponentFindChildByName(component, "sublitframe");	// for Tugboat
	if( !pLitFrameComponent )
	{
		pLitFrameComponent = component;
	}

	BOOL bIsMerchant = InvLocIsStoreLocation(container, invgrid->m_nInvLocation);
	//BOOL bIsMerchant = UnitIsMerchant(container);

	//if (bIsMerchant)
	//{		
	//	invgrid->m_fScrollVertMax = 0.0f;
	//}
	//BOOL bOneSize = InvLocTestFlag(&tInvlocInfo, INVLOCFLAG_ONESIZE);

	UI_COMPONENT *pChild = UIComponentFindChildByName(invgrid, "quantity text");
	UI_LABELEX *pQuantityText = (pChild ? UICastToLabel(pChild) : NULL);
	pChild = UIComponentFindChildByName(invgrid, "price text");
	UI_LABELEX *pPriceText = (pChild && UIIsMerchantScreenUp() ? UICastToLabel(pChild) : NULL);

	UI_COMPONENT *pIconPanel = UIComponentFindChildByName(invgrid, "icon panel");
	UIX_TEXTURE *pIconTexture = (pIconPanel ? UIComponentGetTexture(pIconPanel) : NULL);
	pChild = UIComponentFindChildByName(component, "cooldown text");
	UI_LABELEX *pCooldown = (pChild ? UICastToLabel(pChild) : NULL);
	UI_COMPONENT *pCooldownHighlight = UIComponentFindChildByName(component, "cooldown highlight");

	UIX_TEXTURE *pPriceTexture = NULL;
	UIX_TEXTURE_FONT *pPriceFont = NULL;
	int nPriceFontSize = 0; 

	if (pPriceText)
	{
		pPriceTexture = UIComponentGetTexture(pPriceText);
		pPriceFont = UIComponentGetFont(pPriceText);
		nPriceFontSize = UIComponentGetFontSize(pPriceText); 

		UIComponentRemoveAllElements(pPriceText);
	}
	if (pQuantityText)
	{
		UIComponentRemoveAllElements(pQuantityText);
	}
	if (pIconPanel)
	{
		UIComponentRemoveAllElements(pIconPanel);
	}
	if (pLitFrameComponent)		// for Tugboat
	{
		UIComponentRemoveAllElements(pLitFrameComponent);
	}
	if (pCooldown)
	{
		UIComponentRemoveAllElements(pCooldown);
	}
	if (pCooldownHighlight)
	{
		UIComponentRemoveAllElements(pCooldownHighlight);
	}

	UIX_TEXTURE* texture = UIComponentGetTexture(invgrid);
	ASSERT_RETVAL(texture, UIMSG_RET_NOT_HANDLED);

	UIComponentAddFrame(invgrid);

	int nCursorOverGridX = -1, nCursorOverGridY = -1;
	//UIComponentAddBoxElement(component, 0, 0, component->m_fWidth, component->m_fHeight, NULL, UI_HILITE_GREEN, 64);

	float cx, cy;
	UIGetCursorPosition(&cx, &cy);
	UNIT* pCursorItem = UnitGetById(AppGetCltGame(), UIGetCursorUnit());

	if (!AppIsTugboat() && pCursorItem)
	{
		// make the apparent cursor position the top-left of the item.
		ASSERT(g_UI.m_Cursor);
		cx += g_UI.m_Cursor->m_tItemAdjust.fXOffset + (invgrid->m_fCellWidth / 2.0f);
		cy += g_UI.m_Cursor->m_tItemAdjust.fYOffset + (invgrid->m_fCellHeight / 2.0f);
	}

	UI_POSITION pos;
	UNIT * pItemUnderCursor = NULL;
	if (UIComponentCheckBounds(invgrid, cx, cy, &pos))
	{
		pos.m_fX -= UIComponentGetScrollPos(invgrid).m_fX;
		pos.m_fY -= UIComponentGetScrollPos(invgrid).m_fY;
		nCursorOverGridX = (int)((cx - pos.m_fX) / UIGetScreenToLogRatioX(invgrid->m_bNoScale) / (invgrid->m_fCellWidth + invgrid->m_fCellBorder));
		//nCursorOverGridX = PIN(nCursorOverGridX, 0, info.nWidth - 1);
		nCursorOverGridY = (int)((cy - pos.m_fY) / UIGetScreenToLogRatioY(invgrid->m_bNoScale) / (invgrid->m_fCellHeight + invgrid->m_fCellBorder));
		//nCursorOverGridY = PIN(nCursorOverGridY, 0, info.nHeight - 1);

		pItemUnderCursor = UnitInventoryGetByLocationAndXY(container, invgrid->m_nInvLocation, nCursorOverGridX, nCursorOverGridY);
	}

	UI_RECT rectComponent = UIComponentGetRectBase(invgrid);
	BOOL UnitHighlighted = FALSE;
	UNIT* InventoryHighlightItem = NULL;

	// If the cursor is inside the grid
	if (nCursorOverGridX != -1 && nCursorOverGridY != -1)
	{
		// and there's an item in the cursor
		if (pCursorItem && pCursorItem != pItemUnderCursor)
		{
			// find the unit at that grid box
			if (InventoryGridGetItemsOverlap(container, pCursorItem, pItemUnderCursor, invgrid->m_nInvLocation, nCursorOverGridX, nCursorOverGridY) <= 1)
			{
				// if the cursor item can go in this grid box
				if (InventoryLocPlayerCanPut(container, pCursorItem, invgrid->m_nInvLocation) && 
					UnitInventoryTestIgnoreItem(container, pCursorItem, pItemUnderCursor, invgrid->m_nInvLocation, nCursorOverGridX, nCursorOverGridY))
				{
					if (invgrid->m_pItemBkgrdFrame)
					{
						float gx = (float)nCursorOverGridX * (invgrid->m_fCellWidth + invgrid->m_fCellBorder);
						float gy = (float)nCursorOverGridY * (invgrid->m_fCellHeight + invgrid->m_fCellBorder);
						if (!AppIsTugboat())
					    {
						    UI_SIZE tItemSizeInGrid = sGetItemSizeInGrid(pCursorItem, invgrid, &tInvlocInfo);
						    UIComponentAddElement(pLitFrameComponent, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), GetFontColor(FONTCOLOR_HILIGHT_BKGD), &rectComponent, TRUE, 1.0f, 1.0f, &tItemSizeInGrid);
					    }
						else
				        {
							UI_SIZE sizeIcon;
							sizeIcon.m_fWidth = invgrid->m_fCellWidth * pCursorItem->pUnitData->nInvWidth;
							sizeIcon.m_fHeight = invgrid->m_fCellHeight * pCursorItem->pUnitData->nInvHeight;

					        UIComponentAddElement(pLitFrameComponent, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), GFXCOLOR_WHITE, &rectComponent, FALSE, 1.0f, 1.0f, &sizeIcon );
							UnitHighlighted = TRUE;
							InventoryHighlightItem = pItemUnderCursor;
				        }
					}
				}
			}
			else
			{
				pItemUnderCursor = NULL;
			}
		}
		else
		{
			UNIT * pItem = UnitInventoryGetByLocationAndXY(container, invgrid->m_nInvLocation, nCursorOverGridX, nCursorOverGridY);
			if (InventoryLocPlayerCanTake(container, pItem, invgrid->m_nInvLocation))
			{
				InventoryHighlightItem = pItem;
				UnitHighlighted = TRUE;
			}
		}
	}
	BOOL bIconsNotLoaded = FALSE;
	BOOL bCooling = FALSE;
	BOOL bInvGridActive = UIComponentGetActive(invgrid);
	UNITID idModdingItem = UIGetModUnit();
	UNIT *pModdingItem = (idModdingItem != INVALID_ID ? UnitGetById(AppGetCltGame(), idModdingItem) : NULL);

	UNIT* item = NULL;
	while ((item = UnitInventoryLocationIterate(container, invgrid->m_nInvLocation, item, 0, FALSE)) != NULL)
	{
		if (!AppIsTugboat() && UnitIsWaitingForInventoryMove(item))
		{
			// just don't draw for now.  Maybe ghost it out later.
			continue;
		}
		BOOL Highlighted = FALSE;

		if( UnitHighlighted &&
			InventoryHighlightItem == item )
		{
			Highlighted = TRUE;
		}

		const UNIT_DATA* item_data = ItemGetData(item);
		ASSERT_CONTINUE(item_data);

		BOOL bLoaded = TRUE;
		if ( item->pIconTexture )
		{
			bLoaded = UITextureLoadFinalize( item->pIconTexture );
			REF( bLoaded );
			if( !bLoaded )
			{
				bIconsNotLoaded = TRUE;
			}
		}

		int x, y;
		UnitGetInventoryLocation(item, NULL, &x, &y);

		float gx = (float)x * (invgrid->m_fCellWidth + invgrid->m_fCellBorder);
		float gy = (float)y * (invgrid->m_fCellHeight + invgrid->m_fCellBorder);
		UI_SIZE tItemSizeInGrid = sGetItemSizeInGrid(item, invgrid, &tInvlocInfo);
		UI_SIZE *pItemSizeInGrid = &tItemSizeInGrid;		

		int iScale = -1;
		if (x >= 0 && x < invgrid->m_nGridWidth && y >= 0 && y < invgrid->m_nGridHeight)
		{
			iScale = (y * invgrid->m_nGridWidth) + x;
		}

		//if (bIsMerchant)
		//{
		//	int nItemInvHeight = (bOneSize ? 1 : MAX(1, UnitGetStat(item, STATS_INVENTORY_HEIGHT)));
		//	invgrid->m_fScrollVertMax = MAX(invgrid->m_fScrollVertMax, ((y+nItemInvHeight) * (invgrid->m_fCellHeight + invgrid->m_fCellBorder)) - invgrid->m_fHeight);
		//}

		// If the cursor is over this item
		if (item == pItemUnderCursor)
		{
			if (invgrid->m_pItemBkgrdFrame)
			{
				if( AppIsHellgate() )
				{
					UIComponentAddElement(component, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), GetFontColor(FONTCOLOR_HILIGHT_BKGD), &rectComponent, TRUE, 1.0f, 1.0f, &tItemSizeInGrid);
				}
				else if( AppIsTugboat() )
				{
					UIComponentAddElement(component, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), GFXCOLOR_WHITE, &rectComponent, TRUE, 1.0f, 1.0f, &tItemSizeInGrid);
				}
				Highlighted = TRUE;
			}
		}
		else if (UIGetCursorState() == UICURSOR_STATE_SKILL_PICK_ITEM)
		{
			int nSkillID = UIGetCursorActivateSkill();
			if (nSkillID != INVALID_ID)
			{
				GAME *pGame = AppGetCltGame();	ASSERT(pGame);
				UNIT *pPlayer = GameGetControlUnit(pGame);	ASSERT(pPlayer);
				if (SkillIsValidTarget(pGame, pPlayer, item, NULL, nSkillID, FALSE ))
				{
					UIComponentAddElement(pLitFrameComponent, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), GFXCOLOR_GREEN, &rectComponent, TRUE, 1.0f, 1.0f, pItemSizeInGrid);
				}
			}
		}
		else if ((pCursorItem && UnitIsA(pCursorItem, UNITTYPE_MOD) && !ItemBelongsToAnyMerchant(item)) ||
				  pModdingItem != NULL)
		{
			//Highlight this item if the mod can go in it
			if( AppIsHellgate() && pIconPanel)
			{
				UNIT *pItemToCheck = (pModdingItem ? pModdingItem : item);
				UNIT *pModItem = (pModdingItem ? item : pCursorItem);

				int nLocation = INVLOC_NONE;
				BOOL bOpenSpace = ItemGetOpenLocation(pItemToCheck, pModItem, FALSE, &nLocation, NULL, NULL);
				if (nLocation != INVLOC_NONE &&
					UnitIsA(pModItem, UNITTYPE_MOD))
				{
					BOOL bMeetsRequirements = InventoryCheckStatRequirements(pItemToCheck, pModItem, NULL, NULL, nLocation);
					if (!bOpenSpace || !bMeetsRequirements)
					{
						UIComponentAddPrimitiveElement(pIconPanel, UIPRIM_BOX, 0, UI_RECT(gx, gy, gx+tItemSizeInGrid.m_fWidth, gy+tItemSizeInGrid.m_fHeight), NULL, &rectComponent, invgrid->m_dwModWontFitColor);
					}
				}
				else
				{
					UIComponentAddPrimitiveElement(pIconPanel, UIPRIM_BOX, 0, UI_RECT(gx, gy, gx+tItemSizeInGrid.m_fWidth, gy+tItemSizeInGrid.m_fHeight), NULL, &rectComponent, invgrid->m_dwModWontGoInItemColor);
				}
			}
			
		}

		if (pIconPanel && !bInvGridActive)
		{
			// gray out the items if the grid is not active
			UI_RECT rectClip = rectComponent;		// make a copy
			UIComponentCheckClipping(invgrid, rectClip);

			UIComponentAddPrimitiveElement(pIconPanel, UIPRIM_BOX, 0, UI_RECT(gx, gy, gx+tItemSizeInGrid.m_fWidth, gy+tItemSizeInGrid.m_fHeight), NULL, &rectClip, invgrid->m_dwInactiveItemColor);
		}

		if( AppIsTugboat() &&
 			!Highlighted && item )
		{
			if (invgrid->m_pItemBkgrdFrame)
			{
				DWORD dwColor = GFXCOLOR_WHITE;
				int color = UIInvGetItemGridBackgroundColor(item);
				if (color > 0)
				{
					dwColor = GetFontColor(color);
				}
				if( dwColor != GFXCOLOR_WHITE && !( ItemBelongsToAnyMerchant( item ) && pActualMerchant && UnitIsGambler( pActualMerchant ) ) )
				{
					
					UIComponentAddElement(pLitFrameComponent, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), dwColor, &rectComponent, FALSE, 1.0f, 1.0f, pItemSizeInGrid );
				}
			}
		}
		// don't draw the item if it's referenced in the cursor
		//   or it's a merchant and they have an unlimited stock of the item.
		if (( AppIsTugboat() || UnitGetId(item) != UIGetCursorUnit() ) ||
			(bIsMerchant && UnitGetStat(item, STATS_UNLIMITED_IN_MERCHANT_INVENTORY) != 0))
		{
			UI_RECT gridcellrect;

			if (!AppIsTugboat())
		    {
				// draw background for slot
				int nBkgdColor = UIInvGetItemGridBackgroundColor(item);
				if (nBkgdColor != INVALID_ID)
				{
					UIComponentAddElement(component, texture, invgrid->m_pItemBkgrdFrame, UI_POSITION(gx, gy), GetFontColor(nBkgdColor), &rectComponent, TRUE, 1.0f, 1.0f, pItemSizeInGrid);
				}
				UI_TEXTURE_FRAME *pIconFrame = NULL;
				int nColorIcon;
				int nBorderColor = UIInvGetItemGridBorderColor(item, invgrid, &pIconFrame, &nColorIcon);
				if (nBorderColor != INVALID_ID &&
					invgrid->m_pUseableBorderFrame)
				{
					UIComponentAddElement(component, texture, invgrid->m_pUseableBorderFrame, UI_POSITION(gx, gy), GetFontColor(nBorderColor), &rectComponent, TRUE, 1.0f, 1.0f, pItemSizeInGrid);
				}

			    gridcellrect.m_fX1 = gx + invgrid->m_fCellBorder;
			    gridcellrect.m_fY1 = gy + invgrid->m_fCellBorder;
			    gridcellrect.m_fX2 = gx + tItemSizeInGrid.m_fWidth - invgrid->m_fCellBorder;
			    gridcellrect.m_fY2 = gy + tItemSizeInGrid.m_fHeight - invgrid->m_fCellBorder;
    
				if (pIconFrame && pIconPanel)
				{
					if (pIconTexture)
					{
						UI_POSITION posIcon(gridcellrect.m_fX2 - (pIconFrame->m_fWidth + 5.0f), gridcellrect.m_fY2 - (pIconFrame->m_fHeight + 5.0f));
						UI_RECT rectComponent = UIComponentGetRect(pIconPanel);
						// adjust the clipping for the scroll position
						rectComponent.m_fX1 += UIComponentGetScrollPos(invgrid).m_fX;
						rectComponent.m_fX2 += UIComponentGetScrollPos(invgrid).m_fX;
						rectComponent.m_fY1 += UIComponentGetScrollPos(invgrid).m_fY;
						rectComponent.m_fY2 += UIComponentGetScrollPos(invgrid).m_fY;
						UIComponentAddElement(pIconPanel, pIconTexture, pIconFrame, posIcon, GetFontColor(nColorIcon), &rectComponent, TRUE);
					}
				}

				UIComponentAddItemGFXElement(component, item, gridcellrect, &rectComponent, (iScale >= 0 ? &(invgrid->m_pItemAdj[iScale]) : NULL));
		    }
			else
			{
			    gridcellrect.m_fX1 = gx;
			    gridcellrect.m_fY1 = gy;
			    gridcellrect.m_fX2 = gx + invgrid->m_fCellWidth * item->pUnitData->nInvWidth;
			    gridcellrect.m_fY2 = gy + invgrid->m_fCellHeight * item->pUnitData->nInvHeight;
    
			    UI_GFXELEMENT* element = NULL;
			    if( item->pIconTexture )
			    {
					BOOL bLoaded = UITextureLoadFinalize( item->pIconTexture );
					REF( bLoaded );
				    UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(item->pIconTexture->m_pFrames, "icon");
				    if (frame && bLoaded)
					{
						float OffsetX =  invgrid->m_fCellWidth * item->pUnitData->nInvWidth / 2 - item->pUnitData->nInvWidth * invgrid->m_fCellWidth  / 2;
						float OffsetY =  invgrid->m_fCellHeight * item->pUnitData->nInvHeight / 2 - item->pUnitData->nInvHeight * invgrid->m_fCellHeight / 2;

						BOOL bMeetsRequirements = InventoryCheckStatRequirements(UIGetControlUnit(), item, NULL, NULL, INVLOC_NONE);
						if( bMeetsRequirements )
						{
							bMeetsRequirements = InventoryItemMeetsSkillGroupReqs( item, UIGetControlUnit() );
						}
						if( ItemBelongsToAnyMerchant( item ) && pActualMerchant && UnitIsGambler( pActualMerchant ) )
						{
							bMeetsRequirements = TRUE;
						}
						DWORD dwColor = bMeetsRequirements ? GFXCOLOR_WHITE : GFXCOLOR_RED;
						
					    element = UIComponentAddElement(component, item->pIconTexture, frame, UI_POSITION(gx + OffsetX, gy + OffsetY) , dwColor, &gridcellrect, 0, 1.0f, 1.0f, pItemSizeInGrid  );//, GetFontColor(pSkillData->nIconColor), NULL, FALSE, sSmallSkillIconScale());
					}
					else
					{
						bIconsNotLoaded = TRUE;
					}
			    }
			    else if( c_UnitGetModelIdInventory( item ) != INVALID_ID )
			    {
				    element = UIComponentAddItemGFXElement(component, item, gridcellrect, &rectComponent);
			    }
			    if (UnitGetId(item) == UIGetCursorUnit() && element)
			    {
				    element->m_dwColor = UI_ITEM_GRAY;
			    }

				//draw the gems and mod slots for tugboat
				sUIX_DrawGemsAndMods( invgrid,
									  NULL,
					                  item );

			}

			// let's show the cooldowns for items too!
			if( AppIsTugboat() )
			{
				int nCooldownTicks = 0;
				// check for can't use item
				UNIT *pTargetItem = UnitGetById( AppGetCltGame(), UIGetCursorUnit() );
				DWORD nFlags = ( g_UI.m_Cursor->m_eCursorState == UICURSOR_STATE_ITEM_PICK_ITEM )?ITEM_USEABLE_MASK_ITEM_TEST_TARGETITEM:ITEM_USEABLE_MASK_NONE;
				if (container && item && ItemIsUsable( container, item, pTargetItem, nFlags ) != USE_RESULT_OK &&
					UnitGetData(item)->nSkillWhenUsed != INVALID_ID )
				{
					float fScale = 1.0f;
					BOOL bIconIsRed = FALSE;
					c_SkillGetIconInfo( container, UnitGetData(item)->nSkillWhenUsed, bIconIsRed, fScale, nCooldownTicks, TRUE );

					if( fScale != 1 )
					{
						UI_RECT componentrect = gridcellrect;
						componentrect.m_fX1;// *= UIGetScreenToLogRatioX( pCooldownHighlight->m_bNoScale );
						componentrect.m_fX2;// *= UIGetScreenToLogRatioX( pCooldownHighlight->m_bNoScale );
						componentrect.m_fY1;// *= UIGetScreenToLogRatioY( pCooldownHighlight->m_bNoScale );
						componentrect.m_fY2;// *= UIGetScreenToLogRatioY( pCooldownHighlight->m_bNoScale );

						float Height = gridcellrect.m_fY2 - gridcellrect.m_fY1;

						UI_GFXELEMENT* pElement = UIComponentAddBoxElement(pCooldownHighlight,componentrect.m_fX1, componentrect.m_fY1 + ( Height) * fScale , componentrect.m_fX2 , componentrect.m_fY2, NULL, UI_HILITE_GRAY, 180, NULL);
						if( pElement )
						{
							pElement->m_fZDelta = -.0002f;
						}

					}

				}

				// if a cooldown is in effect for this slot
				if (nCooldownTicks > 0)
				{
					bCooling = TRUE;

					// display time left
					WCHAR szBuff[32] = { L"0" };
					int nDurationSeconds = (nCooldownTicks + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND;
					LanguageGetDurationString(szBuff, arrsize(szBuff), nDurationSeconds);

					UI_RECT rectComponent = UIComponentGetRect(pCooldown);
					// adjust the clipping for the scroll position
					rectComponent.m_fX1 += UIComponentGetScrollPos(invgrid).m_fX;
					rectComponent.m_fX2 += UIComponentGetScrollPos(invgrid).m_fX;
					rectComponent.m_fY1 += UIComponentGetScrollPos(invgrid).m_fY;
					rectComponent.m_fY2 += UIComponentGetScrollPos(invgrid).m_fY;
					UIComponentAddTextElement(pCooldown, UIComponentGetTexture(pCooldown), 
						UIComponentGetFont(pCooldown), UIComponentGetFontSize(pCooldown), 
						szBuff, UI_POSITION(gx, gy), pCooldown->m_dwColor, &rectComponent, pCooldown->m_nAlignment, pItemSizeInGrid);

				}
			}

			if (pQuantityText)
			{
				if( UnitIsA( item, UNITTYPE_MAP_ITEM_SPAWNER ) )
				{
					int nLevelAreaID = MYTHOS_MAPS::GetLevelAreaID( item );					
					if( nLevelAreaID != INVALID_ID )
					{
						if( MYTHOS_LEVELAREAS::LevelAreaIsHostile( nLevelAreaID ) )
						{
							int nPlayerLevel = UnitGetStat( UIGetControlUnit(), STATS_LEVEL );
							WCHAR szBuff[ 32 ];
							int nMin = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID );
							int nMax = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID );
							PStrPrintf( szBuff, 32, L"%d", nMin );
							UI_RECT rectComponent = UIComponentGetRect(pQuantityText);
							// adjust the clipping for the scroll position
							rectComponent.m_fX1 += UIComponentGetScrollPos(invgrid).m_fX;
							rectComponent.m_fX2 += UIComponentGetScrollPos(invgrid).m_fX;
							rectComponent.m_fY1 += UIComponentGetScrollPos(invgrid).m_fY;
							rectComponent.m_fY2 += UIComponentGetScrollPos(invgrid).m_fY;
							DWORD dwColor = GFXCOLOR_WHITE;
							if( nMax < nPlayerLevel )
							{
								dwColor = GFXCOLOR_GRAY;
							}
							else if( nMin > nPlayerLevel )
							{
								dwColor = GFXCOLOR_RED;
							}
							UIComponentAddTextElement(pQuantityText, UIComponentGetTexture(pQuantityText), 
								UIComponentGetFont(pQuantityText), UIComponentGetFontSize(pQuantityText), 
								szBuff, UI_POSITION(gx, gy), dwColor, &rectComponent, UIALIGN_BOTTOMRIGHT, pItemSizeInGrid);
						}
					}
				}
				else
				{
					int nQuantity = ItemGetQuantity( item );
					if (nQuantity > 1)
					{
						const int MAX_STRING = 32;
						WCHAR szBuff[ MAX_STRING ];
						LanguageFormatIntString( szBuff, MAX_STRING, nQuantity );
						UI_RECT rectComponent = UIComponentGetRect(pQuantityText);
						// adjust the clipping for the scroll position
						rectComponent.m_fX1 += UIComponentGetScrollPos(invgrid).m_fX;
						rectComponent.m_fX2 += UIComponentGetScrollPos(invgrid).m_fX;
						rectComponent.m_fY1 += UIComponentGetScrollPos(invgrid).m_fY;
						rectComponent.m_fY2 += UIComponentGetScrollPos(invgrid).m_fY;
						UIComponentAddTextElement(pQuantityText, UIComponentGetTexture(pQuantityText), 
							UIComponentGetFont(pQuantityText), UIComponentGetFontSize(pQuantityText), 
							szBuff, UI_POSITION(gx, gy), pQuantityText->m_dwColor, &rectComponent, pQuantityText->m_nAlignment, pItemSizeInGrid);
					}
				}
			}

			if (pPriceText)
			{
				cCurrency currency = (bIsMerchant ? EconomyItemBuyPrice(item) : EconomyItemSellPrice(item));
				if ( currency.IsZero() == FALSE )
				{
					int nPrice = currency.GetValue( KCURRENCY_VALUE_INGAME );
					const int MAX_MONEY_STRING = 32;
					WCHAR szBuff[ MAX_MONEY_STRING ];
					UIMoneyGetString( szBuff, MAX_MONEY_STRING, nPrice );
										
					DWORD dwColor = pPriceText->m_dwColor;
					if (bIsMerchant && UnitGetCurrency( UIGetControlUnit() ) < currency )
					{
						dwColor = GFXCOLOR_RED;
					}

					float fWidth = 0.0f;
					float fHeight = 0.0f;
					float fCellWidth = invgrid->m_fCellWidth - invgrid->m_fCellBorder - 8.0f;
					float fCellHeight = invgrid->m_fCellHeight - invgrid->m_fCellBorder - 8.0f;
					int nFontSize = nPriceFontSize;
					int nDecrement = -1;

					while (UIElementGetTextLogSize(pPriceFont, nFontSize - (nDecrement > 0 ? nDecrement : 0), 1.0f, invgrid->m_bNoScaleFont, szBuff, &fWidth, &fHeight) &&
						(fWidth > fCellWidth || fHeight > fCellHeight))
					{
						float fRatio = MAX(fHeight / fCellHeight, fWidth / fCellWidth);
						nFontSize = (int)(((float)nFontSize / fRatio) - 0.5);

						nDecrement++;
					}


					UI_RECT rectComponent = UIComponentGetRect(pPriceText);
					// adjust the clipping for the scroll position
					rectComponent.m_fX1 += UIComponentGetScrollPos(invgrid).m_fX;
					rectComponent.m_fX2 += UIComponentGetScrollPos(invgrid).m_fX;
					rectComponent.m_fY1 += UIComponentGetScrollPos(invgrid).m_fY;
					rectComponent.m_fY2 += UIComponentGetScrollPos(invgrid).m_fY;
					UIComponentAddTextElement(pPriceText, pPriceTexture, pPriceFont, nFontSize, 
						szBuff, UI_POSITION(gx, gy), dwColor, &rectComponent, pPriceText->m_nAlignment);
				}
			}
		}
	}

	if( bIconsNotLoaded || bCooling )
	{
		// it's not asynch loaded yet - schedule a repaint so we can take care of it
		if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
		}
		component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_GAME_TICK, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR)component, UIMSG_PAINT, wParam, lParam));

	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetSkillTab(
	 UNIT * pUnit,
	 int nTabIndex )
{
	if ( nTabIndex == INVALID_LINK )
		return GlobalIndexGet( GI_SKILLTAB_UNIVERSAL );

	ASSERT_RETINVALID(nTabIndex >= 0 && nTabIndex < NUM_SKILL_TABS);
	const UNIT_DATA *pUnitData = UnitGetData(pUnit);
	ASSERT_RETINVALID( pUnitData );
	return pUnitData->nSkillTab[nTabIndex];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillPageOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);

	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (!pFocusUnit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UnitGetStat(pFocusUnit, STATS_SKILL_POINTS) > 0)
	{
		g_UI.m_bVisitSkillPageReminder = TRUE;
	}
	else
	{
		g_UI.m_bVisitSkillPageReminder = FALSE;
	}
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);

	if (msg == UIMSG_SETCONTROLSTAT)
	{
		UI_COMPONENT* pTooltip = UIComponentGetByEnum(UICOMP_SKILLTOOLTIP);
		if (pTooltip)
		{
			int nSkill = (int)pTooltip->m_dwData;
			pTooltip->m_dwData = (DWORD) INVALID_LINK;		// reset the cached tooltip skill
			if (UIComponentGetActive(pTooltip))
			{
				UISetHoverTextSkill(component, UIGetControlUnit(), nSkill, NULL, FALSE, TRUE);
			}
		}
	}

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	const UNIT_DATA *pUnitData = UnitGetData(pFocusUnit);
	ASSERT_RETVAL(pUnitData, UIMSG_RET_NOT_HANDLED);

	UIComponentAddFrame(component);

	// Label the tab buttons	(may need to move this to some sort of post-load process)
	for ( UI_COMPONENT *pChild = component->m_pFirstChild; pChild; pChild = pChild->m_pNextSibling )
	{
		if (msg != UIMSG_PAINT)
		{
			// paint the children
			UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
		}

		if (pChild->m_eComponentType != UITYPE_BUTTON)
			continue;

		UI_BUTTONEX *pButton = UICastToButton(pChild);

		int nSkillTab = sGetSkillTab( pFocusUnit, pButton->m_nAssocTab );


		if ( nSkillTab != INVALID_LINK )
		{
			SKILLTAB_DATA * pSkilltabData = (SKILLTAB_DATA *)ExcelGetData(pGame, DATATABLE_SKILLTABS, nSkillTab );
			UIComponentSetVisible(pChild, TRUE);
			if (pChild->m_pFirstChild && 
				pChild->m_pFirstChild->m_eComponentType == UITYPE_LABEL)
			{
				UIComponentSetActive(pChild, TRUE);
#if ISVERSION(DEVELOPMENT)
				const char *pszKey = StringTableGetKeyByStringIndex( pSkilltabData->nDisplayString );
				UILabelSetTextByStringKey( pChild->m_pFirstChild, pszKey );
#else
				UILabelSetText(pChild->m_pFirstChild, StringTableGetStringByIndex(pSkilltabData->nDisplayString));
#endif

				if( AppIsTugboat() )
				{
					UI_PANELEX* pPanel = UICastToPanel( pButton->m_pParent );

					if( pPanel->m_nCurrentTab == pButton->m_nAssocTab )
					{
						UI_LABELEX * pLabel = UICastToLabel( UIComponentFindChildByName( pPanel, "skill pane header" ) );
						UILabelSetText(pLabel, StringTableGetStringByIndex(pSkilltabData->nDisplayString));
						if( pLabel->m_pParent->m_eComponentType == UITYPE_TOOLTIP )
						{
							UITooltipDetermineContents(pLabel->m_pParent);
							UITooltipDetermineSize(pLabel->m_pParent);
						}
					}
				}

			}
		}
		else
		{
			UIComponentSetVisible(pChild, FALSE);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnSetControlUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);

	// The control unit has changed, so we need to re-look up the tab numbers as the class may have changed.
	pGrid->m_bLookupTabNum = TRUE;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline UI_POSITION sGetSkillPosOnPage(
	const SKILL_DATA *pSkillData,
	const UI_SKILLSGRID *pGrid,
	int nCol,
	int nRow )
{
	UI_POSITION pos;
	pos.m_fX = nCol * (pGrid->m_fCellWidth + pGrid->m_fCellBorderX) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f);
	pos.m_fY = nRow * (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f);
	return pos;
}

static inline BOOL sSkillVisibleOnPage(
	int nSkill,
	const SKILL_DATA *pSkillData,
	const UI_SKILLSGRID *pGrid,
	UNIT *pUnit,
	BOOL bDrawOnlyKnown)
{
	if (!(pSkillData &&
			pSkillData->nSkillTab == pGrid->m_nSkillTab &&
			pSkillData->nSkillPageX > INVALID_LINK &&
			pSkillData->nSkillPageY > INVALID_LINK &&
			pSkillData->nSkillPageX < pGrid->m_nCellsX &&
			pSkillData->nSkillPageY < pGrid->m_nCellsY))
	{
		return FALSE;
	}

	BOOL bHasSkill = (SkillGetLevel(pUnit, nSkill) > 0);
	if (!bHasSkill && bDrawOnlyKnown)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISpellsGridOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (msg == UIMSG_SETCONTROLSTAT && !UIComponentGetVisible(component))
		return UIMSG_RET_NOT_HANDLED;


	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);

	UIComponentRemoveAllElements(pGrid);
	UIComponentRemoveAllElements(pGrid->m_pIconPanel);

	if (pGrid->m_nSkillTab > INVALID_LINK || pGrid->m_bLookupTabNum)
	{
		GAME *pGame = AppGetCltGame();
		if (!pGame)
			return UIMSG_RET_NOT_HANDLED;
		UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
		if (!pFocusUnit)
			return UIMSG_RET_NOT_HANDLED;


		ASSERT(pGrid->m_nInvLocation != INVALID_LINK);

		UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
		if (!pTexture)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		if (pGrid->m_bLookupTabNum)
		{
			pGrid->m_nSkillTab = sGetSkillTab( pFocusUnit, (int)pGrid->m_dwData );
			if ( pGrid->m_nSkillTab != INVALID_ID )
				pGrid->m_bLookupTabNum = FALSE;
		}

		float x=0.0f; float y=0.0f;
		UIGetCursorPosition(&x, &y);
		UI_POSITION pos;
		UIComponentCheckBounds(component, x, y, &pos);
		x -= pos.m_fX;
		y -= pos.m_fY;

		BOOL bDrawOnlyKnown = FALSE;
		const SKILLTAB_DATA *pSkillTabData = pGrid->m_nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pGrid->m_nSkillTab) : NULL;
		if (pSkillTabData)
		{
			bDrawOnlyKnown = pSkillTabData->bDrawOnlyKnown;
		}

		int nClass = UnitGetClass( pFocusUnit );
		const UNIT_DATA * player_data = PlayerGetData( pGame, nClass );
		if (!player_data)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		// find all spells we know, put them in a list to distribute when we find out what slots we have.
		int nSkillRows[50];		
		int nSkillGroups[50];
		int nSkillSlots[50];
		int nSpellsKnown = 0;
		int nRowCount = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_SKILLS);
		for (int iRow = 0; iRow < nRowCount; iRow++)
		{
			const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
			if (sSkillVisibleOnPage(iRow, pSkillData, pGrid, pFocusUnit, bDrawOnlyKnown))
			{
				nSkillSlots[nSpellsKnown] = UnitGetStat( pFocusUnit, STATS_SPELL_SLOT, iRow );

				nSkillRows[nSpellsKnown] = iRow;
				nSkillGroups[nSpellsKnown] = pSkillData->pnSkillGroup[ 0 ];
				nSpellsKnown++;
			}
		}

		int nTier = 0;
		int nPlayerLevel = UnitGetExperienceLevel( pFocusUnit );
		int nSlot = 0;
		UI_DRAWSKILLICON_PARAMS tParams(pGrid->m_pIconPanel, UI_RECT(), UI_POSITION(), -1, NULL, NULL, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat() /* Tugboat doesn't do the small icons here*/ );
		tParams.pBackgroundComponent = pGrid;
		if (pSkillTabData)
		{
			tParams.pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
		}
		for( int iLevel = 0; iLevel < (AppIsDemo()?5:player_data->nMaxLevel); iLevel++ )
		{
			int nSlots = 0;
			UI_POSITION slotpos;
			const PLAYERLEVEL_DATA * level_data = PlayerLevelGetData( pGame, player_data->nUnitTypeForLeveling, iLevel );
			if( level_data )
			{
				for( int i = 0; i < MAX_SPELL_SLOTS_PER_TIER; i++ )
				{
					if( level_data->nSkillGroup[i] != INVALID_ID )
					{
						nSlots++;
					}
				}

				int nSkill = 0;
				for( int i = 0; i < MAX_SPELL_SLOTS_PER_TIER; i++ )
				{
					if( level_data->nSkillGroup[i] != INVALID_ID )
					{
						slotpos.m_fX = pGrid->m_fWidth * .5f - 
									   ( (float)( nSlots - 1 ) * .5f * ( pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) ) +
									   ( pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) * nSkill -
									   pGrid->m_fCellWidth * .5f;
						slotpos.m_fY = nTier * ( pGrid->m_fCellWidth + pGrid->m_fCellBorderY * .5f );
						nSkill++;	
						const SKILLGROUP_DATA* pSkillGroupData = (SKILLGROUP_DATA *)ExcelGetData(pGame, DATATABLE_SKILLGROUPS, level_data->nSkillGroup[i] );
						UI_TEXTURE_FRAME* pBkgndIcon = (UI_TEXTURE_FRAME*)StrDictionaryFind(tParams.pSkillTexture->m_pFrames, pSkillGroupData->szBackgroundIcon );
	
						UI_GFXELEMENT* pElement = UIComponentAddElement(pGrid->m_pIconPanel, pBkgndIcon != NULL ? tParams.pSkillTexture : pTexture, pBkgndIcon != NULL ? pBkgndIcon : pGrid->m_pBkgndFrame, slotpos, nPlayerLevel >= iLevel ? GFXCOLOR_WHITE : GFXCOLOR_DKGRAY, NULL);
					
						if( pElement )
						{
							pElement->m_qwData = level_data->nSkillGroup[i];
						}

						// Add some text for this level tier of skills
						UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
						if (!pFont)
						{
							pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
						}
						UI_POSITION textpos( pGrid->m_Position.m_fX - 10, slotpos.m_fY);
						UI_SIZE size(200, pGrid->m_fCellWidth );
		    
						DWORD dwColor = UI_MAKE_COLOR(255, GFXCOLOR_WHITE);
						WCHAR szLevel[256];
						PStrPrintf( szLevel, 20, 
								    GlobalStringGet( GS_LEVEL_WITH_INTEGER ),
									level_data->nLevel );
						UIComponentAddTextElement(component, NULL, pFont, 40, szLevel, textpos, dwColor, NULL, UIALIGN_LEFT, &size );

						for( int j = 0; j < nSpellsKnown; j++ )
						{
							if( nSkillGroups[j] != INVALID_ID &&
								nSkillGroups[j] == level_data->nSkillGroup[i] &&
								nSkillSlots[j] == nSlot )
							{
								tParams.pos = slotpos;
								tParams.nSkill = nSkillRows[j];
								tParams.rect.m_fX1 = slotpos.m_fX;
								tParams.rect.m_fY1 = slotpos.m_fY;													
								tParams.rect.m_fX2 = tParams.rect.m_fX1 + pGrid->m_fCellWidth;
								tParams.rect.m_fY2 = tParams.rect.m_fY1 + pGrid->m_fCellWidth;

								UIDrawSkillIcon(tParams);

								nSkillGroups[j] = INVALID_ID;
								break;
							}
						}
						nSlot++;
					}
				}

				if( nSlots > 0 )
				{
					nTier++;
				}
			}
		}
//		UIComponentAddBoxElement(component, 0, 0, component->m_fWidth, component->m_fHeight, NULL, UI_HILITE_GREEN, 128);

	}

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISpellsHotBarOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (msg == UIMSG_SETCONTROLSTAT && !UIComponentGetVisible(component))
		return UIMSG_RET_NOT_HANDLED;

	UIComponentRemoveAllElements(component);

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);

	BOOL bLeftHotbar = component->m_dwParam == 0;
	if (pGrid->m_nSkillTab > INVALID_LINK || pGrid->m_bLookupTabNum)
	{
		GAME *pGame = AppGetCltGame();
		if (!pGame)
			return UIMSG_RET_NOT_HANDLED;
		UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
		if (!pFocusUnit)
			return UIMSG_RET_NOT_HANDLED;


		ASSERT(pGrid->m_nInvLocation != INVALID_LINK);

		UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
		if (!pTexture)
		{
			return UIMSG_RET_NOT_HANDLED;
		}


		float x=0.0f; float y=0.0f;
		UIGetCursorPosition(&x, &y);
		UI_POSITION pos;
		UIComponentCheckBounds(component, x, y, &pos);
		x -= pos.m_fX;
		y -= pos.m_fY;

		BOOL bDrawOnlyKnown = TRUE;
		int nClass = UnitGetClass( pFocusUnit );
		const UNIT_DATA * player_data = PlayerGetData( pGame, nClass );
		if (!player_data)
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		// find all spells we know, put them in a list to distribute when we find out what slots we have.
		int nSkillRows[50];		
		int nSkillGroups[50];
		int nSkillSlots[50];
		int nSpellsKnown = 0;
		int nRowCount = SkillsGetCount(AppGetCltGame());
		UI_DRAWSKILLICON_PARAMS tParams(pGrid->m_pIconPanel, UI_RECT(), UI_POSITION(), -1, pGrid->m_pBkgndFrame , pGrid->m_pBorderFrame, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat(), FALSE, TRUE, 1.0f, 1.0f, pGrid->m_fIconWidth, pGrid->m_fIconHeight );
		tParams.pBackgroundComponent = pGrid;
		int nOriginalTab = pGrid->m_nSkillTab;
		for( int i = 0; i < 4; i++ )
		{
			int nSkillTab = sGetSkillTab( pFocusUnit, i );
			pGrid->m_nSkillTab = nSkillTab;
			const SKILLTAB_DATA *pSkillTabData = nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, nSkillTab) : NULL;
			

			if (pSkillTabData)
			{
				tParams.pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
			}

			
			for (int iRow = 0; iRow < nRowCount; iRow++)
			{
				const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
				if ( sSkillCanGoInActiveBar(iRow) &&
					 ( !bLeftHotbar || ( bLeftHotbar && SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_LEFT_MOUSE )) ) &&
					 sSkillVisibleOnPage(iRow, pSkillData, pGrid, pFocusUnit, bDrawOnlyKnown))
				{
					nSkillSlots[nSpellsKnown] = UnitGetStat( pFocusUnit, STATS_SPELL_SLOT, iRow );

					nSkillRows[nSpellsKnown] = iRow;
					nSkillGroups[nSpellsKnown] = pSkillData->pnSkillGroup[ 0 ];
					nSpellsKnown++;
				}
			}
		}
		pGrid->m_nSkillTab = nOriginalTab;

		int nVisibleSkill = 0;
		UI_POSITION slotpos;
		float fMaxWidth = 0;
		float fMaxHeight = 0;
		for( int k = 0; k < 4; k++ )
		{
			float y = fMaxHeight;
			int nSkillTab = sGetSkillTab( pFocusUnit, k );
			BOOL bFoundSkill = FALSE;
			int nSkillIdx = 0;
			for( int j = 0; j < nSpellsKnown; j++ )
			{
				const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, nSkillRows[j]);
				if( pSkillData->nSkillTab == nSkillTab )
				{
					if( !bFoundSkill )
					{
						bFoundSkill = TRUE;
						fMaxHeight += ( pGrid->m_fCellHeight + pGrid->m_fCellBorderY );
					}
					slotpos.m_fX = nSkillIdx * ( pGrid->m_fCellWidth + pGrid->m_fCellBorderX );
					slotpos.m_fY = y;
					if( slotpos.m_fX > fMaxWidth )
					{
						fMaxWidth = slotpos.m_fX;
					}
					tParams.pos = slotpos;
					tParams.nSkill = nSkillRows[j];
					tParams.rect.m_fX1 = slotpos.m_fX;
					tParams.rect.m_fY1 = slotpos.m_fY;													
					tParams.rect.m_fX2 = tParams.rect.m_fX1 + 
						(pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : ( pGrid->m_pBkgndFrame->m_fWidth - pGrid->m_pBkgndFrame->m_fXOffset ));

					tParams.rect.m_fY2 = tParams.rect.m_fY1 + 
						(pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fHeight : ( pGrid->m_pBkgndFrame->m_fHeight - pGrid->m_pBkgndFrame->m_fYOffset ));

					UIDrawSkillIcon(tParams);
					nVisibleSkill++;
					nSkillGroups[j] = INVALID_ID;
					nSkillIdx++;
				}				
			}

		}
		fMaxWidth += ( pGrid->m_fCellWidth + pGrid->m_fCellBorderX );
		component->m_Position.m_fY = -fMaxHeight;
		component->m_fHeight = fMaxHeight;
		component->m_fWidth = fMaxWidth;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillsGridDrawDependency(
	UI_SKILLSGRID *pGrid, 
	BOOL bDrawOnlyKnown, 
	UNIT *pFocusUnit)
{
	// TEMP TEMP TEMP
	UIX_TEXTURE *pTexture = UIComponentGetTexture(pGrid->m_pParent->m_pParent);

	static const int MAX_DIST = 5;
	static const int ARRSIZE = (MAX_DIST*2)+1;
	UI_TEXTURE_FRAME * pDepFrames[ARRSIZE][ARRSIZE];
	memclear(&pDepFrames, ARRSIZE * ARRSIZE * sizeof(UI_TEXTURE_FRAME *));

	char szBuf[256];
	for (int x = 0; x < ARRSIZE; x++)
	{
		for (int y = 0; y < ARRSIZE; y++)
		{
			int xRel = x-MAX_DIST;
			int yRel = y-MAX_DIST;
			if (xRel != 0 && yRel != 0)
			{
				PStrPrintf(szBuf, arrsize(szBuf), "line_%d%c_%d%c", abs(xRel), xRel < 0 ? 'l' : 'r', abs(yRel), yRel < 0 ? 'u' : 'd');
			}
			else if (xRel != 0)
			{
				PStrPrintf(szBuf, arrsize(szBuf), "line_%d%c", abs(xRel), xRel < 0 ? 'l' : 'r');
			}
			else if (yRel != 0)
			{
				PStrPrintf(szBuf, arrsize(szBuf), "line_%d%c", abs(yRel), yRel < 0 ? 'u' : 'd');
			}
			pDepFrames[x][y] = (UI_TEXTURE_FRAME *)StrDictionaryFind(pTexture->m_pFrames, szBuf);
		}
	}

	//float fSkillIconWidth = (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : pGrid->m_pBkgndFrame->m_fWidth);
	//float fSkillIconHeight = (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fHeight : pGrid->m_pBkgndFrame->m_fHeight);
	//float fSkillIconOffsetX = (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : pGrid->m_pBkgndFrame->m_fXOffset);
	//float fSkillIconOffsetY = (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : pGrid->m_pBkgndFrame->m_fYOffset);

	float fSkillIconOffsetX = pGrid->m_fIconWidth / 2.0f;
	float fSkillIconOffsetY = pGrid->m_fIconHeight / 2.0f;

	unsigned int nRowCount = SkillsGetCount(AppGetCltGame());
	for (unsigned int iRow = 0; iRow < nRowCount; iRow++)
	{
		const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(NULL, DATATABLE_SKILLS, iRow);
		if (sSkillVisibleOnPage(iRow, pSkillData, pGrid, pFocusUnit, bDrawOnlyKnown))
		{
			UI_POSITION posSkill = sGetSkillPosOnPage(pSkillData, pGrid, pSkillData->nSkillPageX, pSkillData->nSkillPageY );
			//posSkill.m_fX -= fSkillIconOffsetX;
			//posSkill.m_fY -= fSkillIconOffsetY;
			for( int i = 0; i < MAX_PREREQUISITE_SKILLS; i++ )
			{
				if( pSkillData->pnRequiredSkills[i] != INVALID_ID &&
					pSkillData->pnRequiredSkillsLevels[i] > 0 )
				{
					const SKILL_DATA *pReqSkillData = SkillGetData( NULL, pSkillData->pnRequiredSkills[i] );	

					ASSERTONCE_CONTINUE( abs(pSkillData->nSkillPageX - pReqSkillData->nSkillPageX) < MAX_DIST );
					ASSERTONCE_CONTINUE( abs(pSkillData->nSkillPageY - pReqSkillData->nSkillPageY) < MAX_DIST );

					UI_POSITION posReq = sGetSkillPosOnPage(pReqSkillData, pGrid, pReqSkillData->nSkillPageX, pReqSkillData->nSkillPageY );
					UI_TEXTURE_FRAME *pFrame = pDepFrames[pSkillData->nSkillPageX - pReqSkillData->nSkillPageX + MAX_DIST]
														 [pSkillData->nSkillPageY - pReqSkillData->nSkillPageY + MAX_DIST];

					if (pFrame)
					{
						// Just position it directly between the skills
						UI_POSITION posCur( MIN(posSkill.m_fX, posReq.m_fX), MIN(posSkill.m_fY, posReq.m_fY) );
						posCur.m_fX += (fabs(posReq.m_fX - posSkill.m_fX) + pGrid->m_fIconWidth) / 2.0f;
						posCur.m_fY += (fabs(posReq.m_fY - posSkill.m_fY) + pGrid->m_fIconHeight) / 2.0f;
						posCur.m_fX -= pFrame->m_fWidth / 2.0f;
						posCur.m_fY -= pFrame->m_fHeight / 2.0f;
						posCur.m_fX -= fSkillIconOffsetX;
						posCur.m_fY -= fSkillIconOffsetY;

						DWORD dwColor = AppIsHellgate() ? UI_MAKE_COLOR(128, GetFontColor(FONTCOLOR_FSORANGE)) :
							UI_MAKE_COLOR(255, GetFontColor(FONTCOLOR_WHITE));
						UIComponentAddElement(pGrid, pTexture, pFrame, posCur, dwColor);
					}
				}
			}
		}
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define SKILLUP_ANIM_DURATION	1000
#define SKILL_PANEL_MAX_COUNT 100



static void sSkillsGridDrawDependencyTugboat(
	UI_SKILLSGRID *pGrid, 	
	UNIT *pFocusUnit,
	int *skillGridSkillIDs )
{
#define MAX_SKILLS_FOR_LAYOUT 20
	int nSkillsComputed[ MAX_SKILLS_FOR_LAYOUT ];
	int nSkillsFound( 0 );
	//HACK to get line...
	UI_WORLD_MAP *pWorldMap =  UICastToWorldMap( UIComponentGetByEnum(UICOMP_WORLD_MAP) );
	ASSERT_RETURN( pWorldMap );
	UIX_TEXTURE* pTexture = UIComponentGetTexture(pWorldMap->m_pConnectionsPanel);
	ASSERT_RETURN( pTexture );
	UI_TEXTURE_FRAME* pFrame = pWorldMap->m_pLineFrame;
	ASSERT_RETURN( pFrame );
	//end hack	

	for( int nArrayIndex = 0; nArrayIndex < SKILL_PANEL_MAX_COUNT; nArrayIndex++ )
	{
		BOOL drawLines( (skillGridSkillIDs[ nArrayIndex ] > 0) );
		if( drawLines )
		{
			//first make sure we haven't already connected the skill
			for( int nSkillLookFor = 0; nSkillLookFor < nSkillsFound; nSkillLookFor++ )
			{
				if( nSkillsComputed[ nSkillLookFor ] == skillGridSkillIDs[ nArrayIndex ] )
				{
					drawLines = FALSE;
					break;
				}
			}
		}

		if( drawLines )
		{
			//now say we have connected the skill so we don't do it any more...
			nSkillsComputed[ nSkillsFound ++ ] = skillGridSkillIDs[ nArrayIndex ];
			const SKILL_DATA* pSkillData = SkillGetData( NULL, skillGridSkillIDs[ nArrayIndex ]);
			ASSERT( pSkillData );
			if( pSkillData )
			{
				UI_POSITION skillPos = sGetSkillPosOnPage(pSkillData, pGrid, pSkillData->nSkillPageX, pSkillData->nSkillPageY);
				skillPos.m_fX += (pGrid->m_fIconWidth)/2.0f; //center point
				skillPos.m_fX *= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale); 
				skillPos.m_fY += (pGrid->m_fIconHeight)/2.0f;
				skillPos.m_fY *= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale); //center point
				
				int nLastSkillRow = pSkillData->pnSkillLevelToMinLevel[ pSkillData->nMaxLevel - 1 ] -1;
				int nLastRow = nLastSkillRow/SKILL_POINTS_PER_TIER; //find the row in the current grid
				UI_POSITION skillPosBottom = sGetSkillPosOnPage(pSkillData, pGrid, pSkillData->nSkillPageX, nLastRow);
				skillPosBottom.m_fX += (pGrid->m_fIconWidth)/2.0f; //center point
				skillPosBottom.m_fX *= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale); 
				skillPosBottom.m_fY += (pGrid->m_fIconHeight)/2.0f;
				skillPosBottom.m_fY *= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale); //center point

				UIComponentDrawLine( pGrid, 
									 pTexture, 
									 pFrame, 
									 skillPos.m_fX,
									 skillPos.m_fY,
									 skillPosBottom.m_fX,
									 skillPosBottom.m_fY,
									 NULL,
									 GFXCOLOR_WHITE,
									 6);

				for( int nSkillReq = 0; nSkillReq < MAX_PREREQUISITE_SKILLS; nSkillReq++ )
				{
					int nSkillToReq = pSkillData->pnRequiredSkills[ nSkillReq ];
					int nPointsToInvest = max( 1, pSkillData->pnRequiredSkillsLevels[ nSkillReq ] ) - 1;
					if( nSkillToReq != INVALID_ID )
					{						
						const SKILL_DATA* pSkillDataPreReq = SkillGetData( NULL, nSkillToReq);
						ASSERT( pSkillDataPreReq );	
						int nSkillLvl = pSkillDataPreReq->pnSkillLevelToMinLevel[ nPointsToInvest ]; //get the skill
						int nRow = nSkillLvl/SKILL_POINTS_PER_TIER; //find the row in the current grid
						int nCol = pSkillDataPreReq->nSkillPageX;//get the col
						//ok lets draw the lines.....
						//first position of the skill required
						UI_POSITION skillPosReq = sGetSkillPosOnPage(pSkillDataPreReq, pGrid, nCol, nRow);
						skillPosReq.m_fX += pGrid->m_fIconWidth/2.0f;
						skillPosReq.m_fX *= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale); //center point
						skillPosReq.m_fY += pGrid->m_fIconHeight/2.0f;
						skillPosReq.m_fY *= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale); //center point

						//need to find a way to determin if skill can be invested in...
						//Draw horizontal line..
						
						//this needs help
						UIComponentDrawLine( pGrid, 
											 pTexture, 
											 pFrame, 
											 min( skillPos.m_fX, skillPosReq.m_fX ),
											 min( skillPos.m_fY, skillPosReq.m_fY ),
											 max( skillPos.m_fX, skillPosReq.m_fX ),
											 min( skillPos.m_fY, skillPosReq.m_fY ),
											 NULL,
											 GFXCOLOR_WHITE,
											 2 );
						UIComponentDrawLine( pGrid, 
											 pTexture, 
											 pFrame, 
											 skillPos.m_fX,
											 min( skillPos.m_fY, skillPosReq.m_fY ),											 
											 skillPos.m_fX,
											 max( skillPos.m_fY, skillPosReq.m_fY ),
											 NULL,
											 GFXCOLOR_WHITE,
											 2);
						
					}

				}
			}
		}
	}

}
#define TIER_DEPTH 5
#define TIERS_HIGH 10
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnPaintTugboat(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{	

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);

	UIComponentRemoveAllElements(pGrid);
	UIComponentRemoveAllElements(pGrid->m_pIconPanel);
	UIComponentRemoveAllElements(pGrid->m_pHighlightPanel);

	if (pGrid->m_nSkillTab > INVALID_LINK || pGrid->m_bLookupTabNum)
	{
		GAME *pGame = AppGetCltGame();
		if (!pGame)
			return UIMSG_RET_NOT_HANDLED;
		UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
		if (!pFocusUnit)
			return UIMSG_RET_NOT_HANDLED;
		UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
		if (!pTexture)		
			return UIMSG_RET_NOT_HANDLED;
		
		if ( pGrid->m_bLookupTabNum )
		{
			pGrid->m_nSkillTab = sGetSkillTab( pFocusUnit, (int)pGrid->m_dwData );
			if ( pGrid->m_nSkillTab != INVALID_ID )
				pGrid->m_bLookupTabNum = FALSE;
		}
		UI_COMPONENT* pPlusButton = UIComponentGetByEnum(UICOMP_SKILL_INVESTMENT_PLUS_BUTTON);
		
		if( pPlusButton )
		{
			pPlusButton->m_dwParam = pGrid->m_nSkillTab;
		}

		BOOL bDrawOnlyKnown = FALSE;
		const SKILLTAB_DATA *pSkillTabData = pGrid->m_nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pGrid->m_nSkillTab) : NULL;
		if (pSkillTabData)
		{
			bDrawOnlyKnown = pSkillTabData->bDrawOnlyKnown;
		}
		// Tugboat uses an entirely different layout scheme for spells - which are in tabs above 0
		if( bDrawOnlyKnown )
		{
			if( pGrid->m_bIsHotBar )
			{
				return UISpellsHotBarOnPaint( component, msg, wParam, lParam );
			}
			else
			{
				return UISpellsGridOnPaint( component, msg, wParam, lParam );
			}
		}
		UI_COMPONENT* pTierBar = component->m_pParent;
		pTierBar = UIComponentFindChildByName( pTierBar, "skill_tier_bar_vert");


		UI_COMPONENT* pGrayBarPool = component->m_pParent;
		UI_COMPONENT* pCurrentGrayBar = NULL;
		if( pGrayBarPool )
		{
			pGrayBarPool = UIComponentFindChildByName( pGrayBarPool, "skill gray panels");
			ASSERTX_RETVAL( pGrayBarPool, UIMSG_RET_HANDLED, "no gray bars were found for the skill pane" ); 
			pCurrentGrayBar = pGrayBarPool->m_pFirstChild;
		}

		UI_COMPONENT* pBarPool = component->m_pParent;
		UI_COMPONENT* pCurrentBar = NULL;
		if( pBarPool )
		{
			pBarPool = UIComponentFindChildByName( pBarPool, "skill bar panel");
			ASSERTX_RETVAL( pBarPool, UIMSG_RET_HANDLED, "no display bars were found for the skill pane" ); 
			pCurrentBar = pBarPool->m_pFirstChild;
		}

		UI_COMPONENT* pBarLargePool = component->m_pParent;
		UI_COMPONENT* pCurrentBarLarge = NULL;
		if( pBarLargePool )
		{
			pBarLargePool = UIComponentFindChildByName( pBarLargePool, "skill bar panel large");
			ASSERTX_RETVAL( pBarLargePool, UIMSG_RET_HANDLED, "no display BarLarges were found for the skill pane" ); 
			pCurrentBarLarge = pBarLargePool->m_pFirstChild;
		}

		UI_COMPONENT* pArrowPool = component->m_pParent;
		UI_COMPONENT* pCurrentArrow = NULL;
		if( pArrowPool )
		{
			pArrowPool = UIComponentFindChildByName( pArrowPool, "skill arrow panel");
			ASSERTX_RETVAL( pArrowPool, UIMSG_RET_HANDLED, "no arrows were found for the skill pane" ); 
			pCurrentArrow = pArrowPool->m_pFirstChild;
		}

		UI_COMPONENT* pConnecterPool = component->m_pParent;
		UI_COMPONENT* pCurrentConnecter = NULL;
		if( pConnecterPool )
		{
			pConnecterPool = UIComponentFindChildByName( pConnecterPool, "skill line panel");
			ASSERTX_RETVAL( pConnecterPool, UIMSG_RET_HANDLED, "no lines were found for the skill pane" ); 
			pCurrentConnecter = pConnecterPool->m_pFirstChild;
		}

		UI_COMPONENT* pLinePool = component->m_pParent;
		UI_COMPONENT* pCurrentLine = NULL;
		if( pLinePool )
		{
			pLinePool = UIComponentFindChildByName( pLinePool, "skill divider panel");
			ASSERTX_RETVAL( pLinePool, UIMSG_RET_HANDLED, "no arrows were found for the skill pane" ); 
			pCurrentLine = pLinePool->m_pFirstChild;
		}

		int nGridWidth = pGrid->m_nCellsX;
		int nGridHeight = pGrid->m_nCellsY;

		int  skillGridSkillIDs[10][10];
		int  skillGridSkillPoints[10][10];
		int  skillGridSkillPointsInvested[10][10];
		int  skillGridBarHeight[10][10];

		for( int i = 0; i < nGridWidth; i++ )
		{
			for( int j = 0; j < nGridHeight; j++ )
			{
				skillGridSkillIDs[i][j] = INVALID_ID;
				skillGridSkillPoints[i][j] = 0;
				skillGridSkillPointsInvested[i][j] = 0;
				skillGridBarHeight[i][j] = 0;
			}
		}

		int nSkillCount = SkillsGetCount(pGame);
		int nTotalSkillPointsInvested = UnitGetStat(pFocusUnit, STATS_SKILL_POINTS_TIER_SPENT, 0 /*pGrid->m_nSkillTab*/ );

/*		for (int iRow = 0; iRow < nSkillCount; iRow++)
		{
			for( int i = 0; i < 3; i++ )
			{
				int nSkillTab = sGetSkillTab( pFocusUnit, i );

		
				const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
				if (pSkillData &&
					pSkillData->nSkillTab == nSkillTab )
				{
					nTotalSkillPointsInvested += SkillGetLevel(pFocusUnit, iRow);
				}
			}
		}*/

		//first we find all the skills we need on the page and where on the page they belog. As
		//well as how many points each skill can have invested.
		for (int iSkillIndex = 0; iSkillIndex < nSkillCount; iSkillIndex++)
		{
			const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iSkillIndex);

			if (pSkillData &&
				sSkillVisibleOnPage( iSkillIndex, pSkillData, pGrid, pFocusUnit, FALSE ))
			{		
	
				int nInitialLevel = SkillGetSkillLevelValue( pSkillData->pnSkillLevelToMinLevel[0] - 1, KSkillLevel_Tier );
				int nCol = pSkillData->nSkillPageX;
				int nRow = nInitialLevel;
				for( int nLevelIndex = 0; nLevelIndex < MAX_LEVELS_PER_SKILL; nLevelIndex++ ) 
				{					
					if( pSkillData->pnSkillLevelToMinLevel[nLevelIndex] > 0 )
					{
	
						
						
						if( skillGridSkillIDs[ nCol ][ nRow ] == INVALID_ID || 
							skillGridSkillIDs[ nCol ][ nRow ] == iSkillIndex )
						{
							if( skillGridSkillIDs[ nCol ][ nRow ] == INVALID_ID )
							{
								skillGridBarHeight[ nCol ][ nRow ] = 1;
								for( int k = nRow - 1 ; k >= 0; k-- )
								{
									// check if there was a previous bar on this chain, and calculate its
									// size based on how far it is away from this one
									if( skillGridSkillIDs[ nCol ][ k ] != INVALID_ID )
									{
										skillGridBarHeight[ nCol ][ k ] = nRow - k;
										break;
									}
								}
							}
							skillGridSkillIDs[ nCol ][nRow] = iSkillIndex;
							skillGridSkillPoints[ nCol ][nRow]++;
							if( skillGridSkillPointsInvested[ nCol ][nRow] == 0 &&
								nRow != nInitialLevel )//first row is always available
							{
								skillGridSkillPointsInvested[ nCol ][nRow] = nLevelIndex;
							}
						}
					}
				}
			}
		}

		//sSkillsGridDrawDependencyTugboat( pGrid, pFocusUnit, skillGridSkillIDs );

		//now lets draw the skills
		//int nPlayerLevel = UnitGetStat( pFocusUnit, STATS_LEVEL );
		int nColAllowedToInvestIn = SkillGetSkillLevelValue( nTotalSkillPointsInvested, KSkillLevel_Tier );
		BOOL bDrawSkillupButtons = (UnitGetStat(pFocusUnit, STATS_SKILL_POINTS) > 0);

		//figure out where the mouse is.
		float x = 0.0f; 
		float y = 0.0f;
		UIGetCursorPosition(&x, &y);

		UI_POSITION pos;
		UIComponentCheckBounds(component, x, y, &pos);
		x -= pos.m_fX;
		y -= pos.m_fY;
		//get the icon where to draw the text onto...

		// we need logical coords for the element comparisons ( getskillatpos )
		float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
		float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);

		//First setup the skill icon draw info		
		UI_DRAWSKILLICON_PARAMS tParams(pGrid->m_pIconPanel, UI_RECT(), UI_POSITION(), -1, pGrid->m_pBkgndFrame , pGrid->m_pBorderFrame, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat(), FALSE, TRUE, 1.0f, 1.0f, pGrid->m_fIconWidth, pGrid->m_fIconHeight );
		tParams.pBackgroundComponent = pGrid;
		if (pSkillTabData)
		{
			//for the texture in the background - *i think*
			tParams.pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
		}

		// gray out tiers we don't have access to.
		int nTierCounter = 0;

		int nPreviousTier = -1;
		while( nTierCounter <= nTotalSkillPointsInvested && pCurrentGrayBar )
		{
			if( SkillGetSkillLevelValue( nTierCounter, KSkillLevel_Tier ) != nPreviousTier  )
			{
				nPreviousTier = SkillGetSkillLevelValue( nTierCounter, KSkillLevel_Tier );
				UIComponentSetVisible( pCurrentGrayBar, FALSE );
				UIComponentHandleUIMessage(pCurrentGrayBar, UIMSG_PAINT, 0, 0);

				pCurrentGrayBar = pCurrentGrayBar->m_pNextSibling;
			}
			nTierCounter++;

		}
		while( pCurrentGrayBar )
		{
			UIComponentSetActive( pCurrentGrayBar, TRUE );
			UIComponentHandleUIMessage(pCurrentGrayBar, UIMSG_PAINT, 0, 0);
			pCurrentGrayBar = pCurrentGrayBar->m_pNextSibling;
		}
		

		// fill the Tier bar
		UI_BAR* pBar = UICastToBar( UIComponentFindChildByName(pTierBar, "skill bar") );

		int nTiers = 5;
		int nMaxPoints = 30;
		int nTier = SkillGetSkillLevelValue( nTotalSkillPointsInvested, KSkillLevel_Tier );
		float fValue = ( (float)nTier / (float)nTiers ) * 100;
		if( nTotalSkillPointsInvested == nMaxPoints )
		{
			fValue = 100;
		}
		else
		{
			float fNextValue = ( (float)min( nTier + 1, nTiers ) / (float)nTiers ) * 100;
			for( int i = 0; i <= nMaxPoints; i++ )
			{
				if( SkillGetSkillLevelValue( i, KSkillLevel_Tier ) == nTier )
				{

					for( int j = i; j <= nMaxPoints; j++ )
					{
						if( SkillGetSkillLevelValue( j, KSkillLevel_Tier ) != nTier ||
							j == nMaxPoints )
						{
							fValue += ( (float)( nTotalSkillPointsInvested - i) / (float)( j - i ) ) * ( fNextValue - fValue );
							break;
						}
					}
					break;
				}
			}
		}
		
		pBar->m_nValue = (int)fValue;
		pBar->m_nMaxValue = 100;//SKILL_POINTS_PER_TIER_TO_UNLOCK * 10; need to get this from data
					
		UIComponentHandleUIMessage(pTierBar, UIMSG_PAINT, 0, 0);
		// Display a count of the current point invenstment in this pane,
		// and the next tier goal
		if( UIComponentGetVisible( component ) )
		{
			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			if (!pFont)
			{
				pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
			}
			//position
			UI_POSITION textpos(pTierBar->m_Position.m_fX + pTierBar->m_fWidth / 2, pTierBar->m_Position.m_fY  + 370 * g_UI.m_fUIScaler );
			int nFontSize = UIComponentGetFontSize(component);
			UI_SIZE size((pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : ( pGrid->m_pBkgndFrame->m_fWidth - pGrid->m_pBkgndFrame->m_fXOffset )), (float)nFontSize);
			textpos.m_fX -= size.m_fWidth / 2;
			textpos.m_fX -= pGrid->m_Position.m_fX;
			//color
			DWORD dwColor = UI_MAKE_COLOR(255, GFXCOLOR_WHITE);	   
			WCHAR szPoints[256];	
			nFontSize -= 4;
			PStrPrintf(szPoints, arrsize(szPoints), L"%d/%d", min( nMaxPoints, nTotalSkillPointsInvested ), nMaxPoints );
			UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );
			
			int nCurrentTier = 0;
			textpos.m_fX -= 20 * g_UI.m_fUIScaler;

			for( int i = 0; i <= nMaxPoints; i++ )
			{

				if( SkillGetSkillLevelValue( i, KSkillLevel_Tier ) > nCurrentTier )
				{
					float Offset = (float)( nCurrentTier + 1 ) / (float) nTiers * 350 - 10;

					textpos.m_fY = pTierBar->m_Position.m_fY  + Offset * g_UI.m_fUIScaler;
					PStrPrintf(szPoints, arrsize(szPoints), L"%d", i );
					UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );

					nCurrentTier++;							
				}
			}
		}


		for( int nCol = 0; nCol < nGridWidth; nCol++ )
		{	
			int nShownSkillID = INVALID_ID;
			for( int nRow = 0; nRow < nGridHeight; nRow++ )
			{

	
				//ASSERTX_RETVAL( nIndexIntoGrid < SKILL_PANEL_MAX_COUNT, UIMSG_RET_HANDLED, "Error in getting index for skills. Skill index is out of range." ); 
				int nSkillIndex = skillGridSkillIDs[ nCol ][ nRow ];
				if( nSkillIndex > 0 )
				{
					BOOL bSkillDisplayed = FALSE;
					const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, nSkillIndex );
					ASSERTX_RETVAL( pSkillData, UIMSG_RET_HANDLED, "Skill retrieved from Skill grid id's is invalid." ); 
					int nSkillPointsInvested = SkillGetLevel(pFocusUnit, nSkillIndex);
					//if the skills accessible
					int nSkillPointsInvestOnLevel = max( 0, nSkillPointsInvested - skillGridSkillPointsInvested[ nCol ][ nRow ]);
					DWORD nFlags = SkillGetNextLevelFlags( pFocusUnit, nSkillIndex );
					BOOL bSkillAccessible = ( ( SkillGetSkillLevelValue( nTotalSkillPointsInvested, KSkillLevel_Tier ) + 1 ) > (nRow) &&
											  !TESTBIT( nFlags, SKILLLEARN_BIT_MISSING_PREREQUISITE_SKILL ) &&
											  ( skillGridSkillPointsInvested[ nCol ][ nRow ] == 0 ||
											  ( nRow <= nColAllowedToInvestIn &&
												nSkillPointsInvested > 0 &&
												skillGridSkillPointsInvested[ nCol ][ nRow ] <= nSkillPointsInvested )) );

					//position and size
					tParams.rect.m_fX1 = nCol * (pGrid->m_fCellWidth + pGrid->m_fCellBorderX);
					tParams.rect.m_fY1 = nRow * (pGrid->m_fCellHeight + pGrid->m_fCellBorderY);				
					ASSERTX_RETVAL(pGrid->m_pBkgndFrame || pGrid->m_pBorderFrame, UIMSG_RET_NOT_HANDLED, "Skills Grid must have a border frame or background frame to draw properly");
					tParams.rect.m_fX2 = tParams.rect.m_fX1 + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : ( pGrid->m_pBkgndFrame->m_fWidth - pGrid->m_pBkgndFrame->m_fXOffset ));
					tParams.rect.m_fY2 = tParams.rect.m_fY1 + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fHeight : ( pGrid->m_pBkgndFrame->m_fHeight - pGrid->m_pBkgndFrame->m_fYOffset ));
					tParams.pos = sGetSkillPosOnPage(pSkillData, pGrid, nCol, nRow);
					//end position and size
					//is the mouse over it?
					tParams.bMouseOver = UIInRect(UI_POSITION(x,y), tParams.rect);
					//this is the selected frame. Leaving NULL for now.
					tParams.pSelectedFrame = NULL;
					tParams.bGreyout = !bSkillAccessible;
					tParams.fSkillIconWidth = pGrid->m_fIconWidth;
					tParams.fSkillIconHeight = pGrid->m_fIconHeight;
					tParams.nSkill = nSkillIndex;
					//draw the icon
					if( pSkillData->eUsage == USAGE_PASSIVE && pGrid->m_pBkgndFramePassive )
					{
						tParams.pBackgroundFrame = pGrid->m_pBkgndFramePassive;
					}
					else
					{
						tParams.pBackgroundFrame = pGrid->m_pBkgndFrame;
					}
					
					// if a skill was displayed, we need to know to adjust the bar height;
					if( nShownSkillID != nSkillIndex )
					{
						UIDrawSkillIcon(tParams);
						bSkillDisplayed = TRUE;
					}
					
					BOOL bFirst = TRUE;

					if( pCurrentArrow && pCurrentLine && UIComponentGetVisible( component ) )
					{
						for( int k = nRow - 1 ; k >= 0; k-- )
						{
							// check if there was a previous bar on this chain - if so, we don't do this at all
							// size based on how far it is away from this one
							if( skillGridSkillIDs[ nCol ][ k ] == skillGridSkillIDs[ nCol ][ nRow ] )
							{
								bFirst = FALSE;
								break;
							}
						}
						if( bFirst )
						{

							// let's draw arrows from any parents we depend on 
							for( int nSkillReq = 0; nSkillReq < MAX_PREREQUISITE_SKILLS; nSkillReq++ )
							{
								int nSkillToReq = pSkillData->pnRequiredSkills[ nSkillReq ];
								int nPointsToInvest = max( 1, pSkillData->pnRequiredSkillsLevels[ nSkillReq ] ) - 1;
								if( nSkillToReq != INVALID_ID )
								{						
									const SKILL_DATA* pSkillDataPreReq = SkillGetData( NULL, nSkillToReq);
									ASSERT( pSkillDataPreReq );	
									int nSkillLvl = pSkillDataPreReq->pnSkillLevelToMinLevel[ nPointsToInvest ]; //get the skill
									int nReqRow = nSkillLvl/SKILL_POINTS_PER_TIER; //find the row in the current grid
									int nReqCol = pSkillDataPreReq->nSkillPageX;//get the col
									//ok lets draw the lines.....
									//first position of the skill required
									UI_POSITION skillPosReq = sGetSkillPosOnPage(pSkillDataPreReq, pGrid, nReqCol, nReqRow);
									skillPosReq.m_fX += pGrid->m_fIconWidth/2.0f;
									skillPosReq.m_fX *= UIGetScreenToLogRatioX( pGrid->m_bNoScale); //center point
									skillPosReq.m_fY += pGrid->m_fIconHeight/2.0f;
									skillPosReq.m_fY *= UIGetScreenToLogRatioY( pGrid->m_bNoScale); //center point

				
									float ReqX = nReqCol * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (tParams.rect.m_fX2 - tParams.rect.m_fX1 ) / 2;
									float ReqY = nReqRow * ( (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f) ) ;	
									float X = nCol * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (tParams.rect.m_fX2 - tParams.rect.m_fX1 ) / 2;
									float Y = nRow * ( (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f) ) ;	

									float Height = Y - ReqY; 


									if( pCurrentArrow && nReqCol != nCol )
									{

										pCurrentArrow->m_Position.m_fX = X - pCurrentArrow->m_fWidth / 2;
										pCurrentArrow->m_Position.m_fY = ReqY;
										pCurrentArrow->m_fHeight = Height;
										UIComponentSetActive( pCurrentArrow, TRUE );						
										UIComponentHandleUIMessage(pCurrentArrow, UIMSG_PAINT, 0, 0);

										pCurrentArrow = pCurrentArrow->m_pNextSibling;


										pCurrentLine->m_Position.m_fY = ReqY;
										pCurrentLine->m_fWidth = ( abs(nCol - nReqCol) - .5f ) * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (tParams.rect.m_fX2 - tParams.rect.m_fX1 ) / 2;
										// right side 
										if( nCol > nReqCol )
										{
											pCurrentLine->m_Position.m_fX = ReqX  + 5;
										}
										else
										{
											pCurrentLine->m_Position.m_fX = ReqX - pCurrentLine->m_fWidth -  5;
										}
										UIComponentSetActive( pCurrentLine, TRUE );						
										UIComponentHandleUIMessage(pCurrentLine, UIMSG_PAINT, 0, 0);

										pCurrentLine = pCurrentLine->m_pNextSibling;
									}

								}

							}
						}
					}

					// let's show a progress bar for this thing!
					if( pCurrentBar && pCurrentBarLarge && UIComponentGetVisible(component))
					{
						BOOL bLargeIcon = FALSE;
						if( bSkillAccessible && min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]) != skillGridSkillPoints[ nCol ][ nRow ] )
						{
							bLargeIcon = TRUE;
						}
						UI_COMPONENT* pThisBar = bLargeIcon ? pCurrentBarLarge : pCurrentBar;
						// calculate the height and location of the bar.
						float X = nCol * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (pGrid->m_fCellWidth ) / 2 - pThisBar->m_fWidth / 2;
						float Y = ( (float)nRow + .4f ) * ( (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f) ) ;	
						// the following is for the 'line draw' connection method for progress bars
						//float Height =  ( pGrid->m_fCellHeight + pGrid->m_fCellBorderY  + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f));
						//float Height = skillGridBarHeight[ nCol ][ nRow ] * ( pGrid->m_fCellHeight + pGrid->m_fCellBorderY  + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f));
						float Height = pThisBar->m_fHeight;
						// adjust the height and position if a skill icon was shown on this tier
						/*if( bSkillDisplayed )
						{
							Height -= ( tParams.rect.m_fY2 - tParams.rect.m_fY1 );
							Y += ( tParams.rect.m_fY2 - tParams.rect.m_fY1 );
						}*/
						Y += 2;
						Height -= 8;
						// adjust the size and fill of the bar
						if( bSkillAccessible )
						{

							if( pThisBar->m_pFirstChild )
							{
								UI_BAR* pBar = UICastToBar( pThisBar->m_pFirstChild );

								pBar->m_nValue = min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]);
								pBar->m_nMaxValue = skillGridSkillPoints[ nCol ][ nRow ];
							}
							
		

							// set the background's elements to show skill mouseovers
							pThisBar->m_dwData = nSkillIndex;

							pThisBar->m_Position.m_fX = X;
							pThisBar->m_Position.m_fY = Y;
							pThisBar->m_ActivePos = pThisBar->m_Position;
							UIComponentSetActive( pThisBar, TRUE );						
							UIComponentHandleUIMessage(pThisBar, UIMSG_PAINT, 0, 0);

						}

						// draw a line to the next bar, if required
											
						if( skillGridBarHeight[ nCol ][ nRow ] > 0 && pCurrentConnecter )
						{
							BOOL bOnEnd = TRUE;
							for( int k = nRow + 1 ; k < nGridHeight; k++ )
							{
								// check if there was a previous bar on this chain, and calculate its
								// size based on how far it is away from this one
								if( skillGridSkillIDs[ nCol ][ k ] != skillGridSkillIDs[ nCol ][ nRow ] &&
									skillGridSkillIDs[ nCol ][ k ] != INVALID_ID )
								{
									const SKILL_DATA* pSkillDataCheck = (SKILL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLS, skillGridSkillIDs[ nCol ][ k ]);
									if( pSkillDataCheck->pnRequiredSkills[ 0 ] != INVALID_ID )
									{
										bOnEnd = FALSE;
										break;
									}

								}
							}
							if( !bOnEnd )
							{
								pCurrentConnecter->m_Position.m_fX = X - pCurrentConnecter->m_fWidth / 2 + pThisBar->m_fWidth / 2;
								pCurrentConnecter->m_Position.m_fY = Y;// + Height;
								pCurrentConnecter->m_fHeight = ( skillGridBarHeight[ nCol ][ nRow ] ) * ( pGrid->m_fCellHeight + pGrid->m_fCellBorderY  + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f)) - Height * .5f;
								UIComponentSetActive( pCurrentConnecter, TRUE );						
								UIComponentHandleUIMessage(pCurrentConnecter, UIMSG_PAINT, 0, 0);

								pCurrentConnecter = pCurrentConnecter->m_pNextSibling;

							}
						}

						// show current point investment vs Max
						// Add some tiny text to ID the skill
						if( min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]) != skillGridSkillPoints[ nCol ][ nRow ] )
						{
							UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
							if (!pFont)
							{
								pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
							}
							//position
							UI_POSITION textpos(tParams.rect.m_fX1, tParams.rect.m_fY1 + pGrid->m_fCellHeight );
							int nFontSize = UIComponentGetFontSize(component);
							textpos.m_fY = Y + Height / 2 - nFontSize * g_UI.m_fUIScaler + 11 * g_UI.m_fUIScaler;
							UI_SIZE size(pGrid->m_fCellWidth, (float)nFontSize);
							//color
							DWORD dwColor = UI_MAKE_COLOR(200, GFXCOLOR_WHITE);	   
							WCHAR szPoints[256];					
							if( bSkillAccessible )
							{
								PStrPrintf(szPoints, arrsize(szPoints), L"%d/%d", min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]) , skillGridSkillPoints[ nCol ][ nRow ] );
								UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize - 2, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );
							}
							/*else
							{
								textpos.m_fY += 7 * g_UI.m_fUIScaler;
								dwColor = UI_MAKE_COLOR(128, GFXCOLOR_WHITE);	   
								PStrPrintf(szPoints, arrsize(szPoints), L"%d",  skillGridSkillPoints[ nCol ][ nRow ] );
								UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize - 4, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );
							}*/

						}


						DWORD dwFlags = SkillGetNextLevelFlags(pFocusUnit, nSkillIndex);

						//draw the skill up icon
						if ( TESTBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL ) && 
							bDrawSkillupButtons && 
							pGrid->m_pSkillupButtonFrame &&
							bSkillAccessible &&
							nSkillPointsInvestOnLevel < skillGridSkillPoints[ nCol ][ nRow ] )
						{
							DWORD dwColor = pGrid->m_pSkillupButtonFrame->m_dwDefaultColor;
							UI_RECT rectFrame;
							UI_POSITION pos;
							pos.m_fX = X;
							pos.m_fY = Y + ( Height - pGrid->m_pSkillupButtonFrame->m_fHeight ) * g_UI.m_fUIScaler ;
							rectFrame.m_fX1 = pos.m_fX;
							rectFrame.m_fY1 = pos.m_fY;
							rectFrame.m_fX2 = rectFrame.m_fX1 + pGrid->m_pSkillupButtonFrame->m_fWidth;
							rectFrame.m_fY2 = rectFrame.m_fY1 + pGrid->m_pSkillupButtonFrame->m_fHeight;
							UI_TEXTURE_FRAME *pFrame = pGrid->m_pSkillupButtonFrame;
							if (UIInRect(UI_POSITION(logx,logy), rectFrame))
							{
								pFrame = pGrid->m_pSkillupButtonDownFrame;
							}
							UI_GFXELEMENT* pElement = UIComponentAddElement(pGrid->m_pIconPanel, pTexture, pFrame, pos, dwColor);
							if( pElement )
							{
								pElement->m_qwData = nSkillIndex;
							}
							if (pGrid->m_pSkillupBorderFrame)
								UIComponentAddElement(pGrid->m_pHighlightPanel, pTexture, pGrid->m_pSkillupBorderFrame, pos);
						}
						if( bSkillAccessible )
						{
							if( bLargeIcon )
							{
								pCurrentBarLarge = pCurrentBarLarge->m_pNextSibling;
							}
							else
							{
								pCurrentBar = pCurrentBar->m_pNextSibling;
							}
						}
					}
					nShownSkillID = nSkillIndex;
					

				}
			}
		}

		// now hide any remaining bars that weren't used
		while( pCurrentBar )
		{
			UIComponentSetVisible( pCurrentBar, FALSE );
			pCurrentBar = pCurrentBar->m_pNextSibling;
		}
		// now hide any remaining bars that weren't used
		while( pCurrentBarLarge )
		{
			UIComponentSetVisible( pCurrentBarLarge, FALSE );
			pCurrentBarLarge = pCurrentBarLarge->m_pNextSibling;
		}
		// now hide any remaining bars that weren't used
		while( pCurrentBarLarge )
		{
			UIComponentSetVisible( pCurrentBarLarge, FALSE );
			pCurrentBarLarge = pCurrentBarLarge->m_pNextSibling;
		}
		// now hide any remaining arrows that weren't used
		while( pCurrentArrow )
		{
			UIComponentSetVisible( pCurrentArrow, FALSE );
			pCurrentArrow = pCurrentArrow->m_pNextSibling;
		}
		// now hide any remaining connecters that weren't used
		while( pCurrentConnecter )
		{
			UIComponentSetVisible( pCurrentConnecter, FALSE );
			pCurrentConnecter = pCurrentConnecter->m_pNextSibling;
		}
		// now hide any remaining lines that weren't used
		while( pCurrentLine )
		{
			UIComponentSetVisible( pCurrentLine, FALSE );
			pCurrentLine = pCurrentLine->m_pNextSibling;
		}

	}

	return UIMSG_RET_HANDLED;
}


UI_MSG_RETVAL UISkillsGridOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (msg == UIMSG_SETCONTROLSTAT && !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;
	if( AppIsTugboat() )
	{
		if (!UIComponentGetActive(component))
			return UIMSG_RET_NOT_HANDLED;
		//Travis will re-implement this when he gets time to fix the UI scrollbar issue with panes.
		return UISkillsGridOnPaintTugboat( component, msg, wParam, lParam );
	}

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);

	UIComponentRemoveAllElements(pGrid);
	UIComponentRemoveAllElements(pGrid->m_pIconPanel);

	if (pGrid->m_nSkillTab > INVALID_LINK || pGrid->m_bLookupTabNum)
	{
		GAME *pGame = AppGetCltGame();
		if (!pGame)
			return UIMSG_RET_NOT_HANDLED;
		UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
		if (!pFocusUnit)
			return UIMSG_RET_NOT_HANDLED;

		if ( pGrid->m_bLookupTabNum )
		{
			pGrid->m_nSkillTab = sGetSkillTab( pFocusUnit, (int)pGrid->m_dwData );
			if ( pGrid->m_nSkillTab != INVALID_ID )
				pGrid->m_bLookupTabNum = FALSE;
		}


		BOOL bDrawOnlyKnown = FALSE;
		BOOL bIsPerkTab = FALSE;
		const SKILLTAB_DATA *pSkillTabData = pGrid->m_nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pGrid->m_nSkillTab) : NULL;
		if (pSkillTabData)
		{
			bDrawOnlyKnown = pSkillTabData->bDrawOnlyKnown;
			bIsPerkTab = pSkillTabData->bIsPerkTab;
		}
		// Tugboat uses an entirely different layout scheme for spells - which are in tabs above 0
		if( AppIsTugboat() &&
			bDrawOnlyKnown )
		{
			if( pGrid->m_bIsHotBar )
			{
				return UISpellsHotBarOnPaint( component, msg, wParam, lParam );
			}
			else
			{
				return UISpellsGridOnPaint( component, msg, wParam, lParam );
			}
		}
		BOOL bDrawSkillupButtons = bIsPerkTab ? (UnitGetStat(pFocusUnit, STATS_PERK_POINTS) > 0) : (UnitGetStat(pFocusUnit, STATS_SKILL_POINTS) > 0);

		ASSERT(pGrid->m_nInvLocation != INVALID_LINK);

		UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
		if (!pTexture)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		// First draw any dependency arrows
		sSkillsGridDrawDependency(pGrid, bDrawOnlyKnown, pFocusUnit);

		float x=0.0f; float y=0.0f;
		UIGetCursorPosition(&x, &y);
		UI_POSITION pos;
		UIComponentCheckBounds(component, x, y, &pos);
		x -= pos.m_fX;
		y -= pos.m_fY;
		// we need logical coords for the element comparisons ( getskillatpos )
		float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
		float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);

//		UIComponentAddBoxElement(component, 0, 0, component->m_fWidth, component->m_fHeight, NULL, UI_HILITE_GREEN, 128);
		UI_DRAWSKILLICON_PARAMS tParams(pGrid->m_pIconPanel, UI_RECT(), UI_POSITION(), -1, pGrid->m_pBkgndFrame , pGrid->m_pBorderFrame, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat(), FALSE, TRUE, 1.0f, 1.0f, pGrid->m_fIconWidth, pGrid->m_fIconHeight );
		tParams.pBackgroundComponent = pGrid;
		if (pSkillTabData)
		{
			tParams.pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
		}

		unsigned int nRowCount = SkillsGetCount(pGame);
		for (unsigned int iRow = 0; iRow < nRowCount; iRow++)
		{
			const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
			if (pSkillData &&
				sSkillVisibleOnPage(iRow, pSkillData, pGrid, pFocusUnit, bDrawOnlyKnown))
			{
				tParams.rect.m_fX1 = pSkillData->nSkillPageX * (pGrid->m_fCellWidth + pGrid->m_fCellBorderX);
				tParams.rect.m_fY1 = pSkillData->nSkillPageY * (pGrid->m_fCellHeight + pGrid->m_fCellBorderY);
				
				ASSERTX_RETVAL(pGrid->m_pBkgndFrame || pGrid->m_pBorderFrame, UIMSG_RET_NOT_HANDLED, "Skills Grid must have a border frame or background frame to draw properly");

				tParams.rect.m_fX2 = tParams.rect.m_fX1 + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : ( pGrid->m_pBkgndFrame->m_fWidth - pGrid->m_pBkgndFrame->m_fXOffset ));
				tParams.rect.m_fY2 = tParams.rect.m_fY1 + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fHeight : ( pGrid->m_pBkgndFrame->m_fHeight - pGrid->m_pBkgndFrame->m_fYOffset ));

				tParams.bMouseOver = UIInRect(UI_POSITION(x,y), tParams.rect);
				BOOL bHasSkill = (SkillGetLevel(pFocusUnit, iRow) > 0);

				BOOL bSelected = FALSE;
				tParams.pSelectedFrame = NULL;
				if ( bHasSkill )
				{
					bSelected = SkillIsSelected(pGame, pFocusUnit, iRow ) && SkillDataTestFlag( pSkillData, SKILL_FLAG_HIGHLIGHT_WHEN_SELECTED );
					if (bSelected)
						tParams.pSelectedFrame = pGrid->m_pSelectedFrame;
				}

				tParams.pos = sGetSkillPosOnPage(pSkillData, pGrid, pSkillData->nSkillPageX, pSkillData->nSkillPageY);
				tParams.bGreyout = !bHasSkill;
				tParams.nSkill = iRow;

				UIDrawSkillIcon(tParams);

				if ((unsigned int)pGrid->m_nSkillupSkill == iRow)
				{
					DWORD dwElapsed = TimeGetElapsed(pGrid->m_tSkillupAnimInfo.m_tiAnimStart, AppCommonGetCurTime());
					float fScale = ((float)dwElapsed / (float) pGrid->m_tSkillupAnimInfo.m_dwAnimDuration) + 1.0f;
					UI_DRAWSKILLICON_PARAMS tParams2(pGrid->m_pIconPanel, UI_RECT(-UIDefaultWidth(), -UIDefaultHeight(), UIDefaultWidth(), UIDefaultHeight()), tParams.pos, iRow, NULL, NULL, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat(), FALSE, TRUE, fScale, fScale, pGrid->m_fIconWidth, pGrid->m_fIconHeight );

					UIDrawSkillIcon(tParams2);

					if (fScale < 2.0f)
					{
						pGrid->m_tSkillupAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + 10, pGrid, UIMSG_PAINT, 0, 0);	
					}
					else
					{
						pGrid->m_nSkillupSkill = INVALID_ID;
					}
				}
				
				// show current point investment vs Max
				// Add some tiny text to ID the skill
				UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
				if (!pFont)
				{
					pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
				}
				UI_POSITION textpos(tParams.rect.m_fX1, tParams.rect.m_fY1 + pGrid->m_fCellHeight );
				textpos.m_fY += pGrid->m_fLevelTextOffset;

				int nFontSize = UIComponentGetFontSize(component);
				UI_SIZE size(tParams.rect.m_fX2 - tParams.rect.m_fX1, (float)nFontSize);
    
				DWORD dwColor = UI_MAKE_COLOR(255, GFXCOLOR_WHITE);
   
				WCHAR szPoints[256];
				int nLevel = SkillGetLevel( pFocusUnit, iRow );
				if( nLevel > 0  &&
					( AppIsTugboat() || pSkillData->pnSkillLevelToMinLevel[ 1 ] != INVALID_ID ))
				{
					if (AppIsTugboat())
					{
						PStrPrintf(szPoints, arrsize(szPoints), L"%d", nLevel );
					}
					else
					{
						int nMaxLevel = SkillGetMaxLevelForUI( pFocusUnit, iRow );
						PStrPrintf(szPoints, arrsize(szPoints), L"%d/%d", nLevel, nMaxLevel );
					}
					UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );
				}

				DWORD dwFlags = SkillGetNextLevelFlags(pFocusUnit, iRow);
				BOOL bDrawSkillup = TESTBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL ) && 
					bDrawSkillupButtons && 
					pGrid->m_pSkillupButtonFrame;
				BOOL bDrawShiftEnable = (AppIsHellgate() && 
					bHasSkill &&
					pSkillData->nActivatePriority > 0  /*Skill can go in the shift button*/ &&
					pGrid->m_pSkillShiftButtonFrame);
				
				if( AppIsHellgate() &&
					(bDrawShiftEnable || bDrawSkillup))
				{
					if (pGrid->m_pSkillupBorderFrame)
						UIComponentAddElement(pGrid->m_pIconPanel, pTexture, pGrid->m_pSkillupBorderFrame, tParams.pos);
				}

				if (bDrawSkillup)
				{
					DWORD dwColor = pGrid->m_pSkillupButtonFrame->m_dwDefaultColor;
					UI_RECT rectFrame;
					rectFrame.m_fX1 = tParams.pos.m_fX - pGrid->m_pSkillupButtonFrame->m_fXOffset;
					rectFrame.m_fY1 = tParams.pos.m_fY - pGrid->m_pSkillupButtonFrame->m_fYOffset;
					rectFrame.m_fX2 = rectFrame.m_fX1 + pGrid->m_pSkillupButtonFrame->m_fWidth;
					rectFrame.m_fY2 = rectFrame.m_fY1 + pGrid->m_pSkillupButtonFrame->m_fHeight;

					UI_TEXTURE_FRAME *pFrame = pGrid->m_pSkillupButtonFrame;
					if (UIInRect(UI_POSITION(logx,logy), rectFrame))
					{

						pFrame = pGrid->m_pSkillupButtonDownFrame;

					}
					UI_GFXELEMENT* pElement = UIComponentAddElement(pGrid->m_pIconPanel, pTexture, pFrame, tParams.pos, dwColor);
					if( pElement )
					{
						pElement->m_qwData = iRow;
					}
				}

				if (bDrawShiftEnable)
				{
					UI_TEXTURE_FRAME *pFrame = pGrid->m_pSkillShiftButtonFrame;
					if (UnitGetStat(pFocusUnit, STATS_SKILL_SHIFT_ENABLED, iRow) != 0)
					{
						pFrame = pGrid->m_pSkillShiftButtonDownFrame;
					}
					UI_GFXELEMENT* pElement = UIComponentAddElement(pGrid->m_pIconPanel, pTexture, pFrame, tParams.pos);
					if( pElement )
					{
						pElement->m_qwData = (1 << 16 | iRow);
					}
				}
			}
		}
	}

	return UIMSG_RET_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sSkillsGridGetSkillAtPosition(
	UI_SKILLSGRID *pGrid,
	float x, 
	float y,
	UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return 0;
	}

	UI_GFXELEMENT *pChild = pGrid->m_pIconPanel->m_pGfxElementFirst;
	while(pChild)
	{

		if ( UIElementCheckBounds(pChild, x, y ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID &&
			 (pChild->m_qwData >> 16 == 0))
		{
			if ( (AppIsHellgate() ||
				  pChild->m_fZDelta != 0) ||
				  AppIsTugboat() )
			{
				pos.m_fX = pChild->m_Position.m_fX;
				pos.m_fY = pChild->m_Position.m_fY;

				return (int)(pChild->m_qwData & 0xFFFF) ;
			}
		}
		pChild = pChild->m_pNextElement;
	}
	return INVALID_LINK;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sSkillsGridGetSkillBarAtPosition(
								  UI_COMPONENT* pComponent,
								  float x, 
								  float y,
								  UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}


	UI_COMPONENT *pChild = pComponent->m_pFirstChild;
	while(pChild)
	{

		UI_POSITION pos2 = UIComponentGetAbsoluteLogPosition(pChild);

		UI_RECT componentrect = UIComponentGetRect(pChild);

		if ( !UIComponentGetVisible( pChild ) ||
			x <	 pos2.m_fX + componentrect.m_fX1 || 
			x >= pos2.m_fX + componentrect.m_fX2 ||
			y <  pos2.m_fY + componentrect.m_fY1 ||
			y >= pos2.m_fY + componentrect.m_fY2 )
		{
			pChild = pChild->m_pNextSibling;
			continue;
		}

		if ( (pChild->m_dwData) != (DWORD)INVALID_ID )
		{

			pos.m_fX = pChild->m_Position.m_fX;
			pos.m_fY = pChild->m_Position.m_fY;

			return (int)pChild->m_dwData;
		}
		pChild = pChild->m_pNextSibling;
	}
	return INVALID_LINK;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sSkillsGridGetLevelUpAtPosition(
	UI_SKILLSGRID *pGrid,
	float x, 
	float y,
	UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return 0;
	}

	UI_GFXELEMENT *pChild = pGrid->m_pIconPanel->m_pGfxElementFirst;
	while(pChild)
	{

		if (UIElementCheckBounds(pChild, x, y ) && (pChild->m_qwData)  != (QWORD)INVALID_ID &&
			pChild->m_fZDelta == 0 )
		{
			pos.m_fX = pChild->m_Position.m_fX;
			pos.m_fY = pChild->m_Position.m_fY;
			return (int)(pChild->m_qwData & 0xFFFF) ;
		}
		pChild = pChild->m_pNextElement;
	}
	return INVALID_LINK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sSkillsGridGetSkillLevelUpAtPosition(
	UI_SKILLSGRID *pGrid,
	float x, 
	float y)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return INVALID_LINK;
	}

	if (!pGrid->m_pSkillupButtonFrame)
	{
		return INVALID_LINK;
	}

	if (pGrid->m_nSkillTab > INVALID_LINK)
	{
		BOOL bDrawOnlyKnown = FALSE;
		const SKILLTAB_DATA *pSkillTabData = pGrid->m_nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pGrid->m_nSkillTab) : NULL;
		if (pSkillTabData)
		{
			bDrawOnlyKnown = pSkillTabData->bDrawOnlyKnown;
		}

		int nRowCount = SkillsGetCount(pGame);
		for (int iRow = 0; iRow < nRowCount; iRow++)
		{
			const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
			if (sSkillVisibleOnPage(iRow, pSkillData, pGrid, pFocusUnit, bDrawOnlyKnown))
			{
				DWORD dwFlags = SkillGetNextLevelFlags(pFocusUnit, iRow);
				BOOL bCanLevel = TESTBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL );
				if (bCanLevel)
				{
					UI_POSITION pos = sGetSkillPosOnPage(pSkillData, pGrid, pSkillData->nSkillPageX, pSkillData->nSkillPageY);
					UI_RECT rectFrame;
					rectFrame.m_fX1 = pos.m_fX - pGrid->m_pSkillupButtonFrame->m_fXOffset;
					rectFrame.m_fY1 = pos.m_fY - pGrid->m_pSkillupButtonFrame->m_fYOffset;
					rectFrame.m_fX2 = rectFrame.m_fX1 + pGrid->m_pSkillupButtonFrame->m_fWidth;
					rectFrame.m_fY2 = rectFrame.m_fY1 + pGrid->m_pSkillupButtonFrame->m_fHeight;
					if (UIInRect(UI_POSITION(x,y), rectFrame))
					{
						return iRow;
					}
				}
			}
		}
	}

	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sSkillsGridGetSkillShiftToggleAtPosition(
	UI_SKILLSGRID *pGrid,
	float x, 
	float y)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return 0;
	}

	UI_GFXELEMENT *pChild = pGrid->m_pIconPanel->m_pGfxElementFirst;
	while(pChild)
	{
		if (UIElementCheckBounds(pChild, x, y ) && 
			pChild->m_qwData != (QWORD)INVALID_ID &&
			pChild->m_qwData >> 16 == 0x1 && 
			pChild->m_fZDelta == 0 )
		{
			return (int)(pChild->m_qwData & 0xFFFF) ;
		}
		pChild = pChild->m_pNextElement;
	}
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);
	float x = (float)wParam;
	float y = (float)lParam;
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	// In Mythos, because our skill panes scroll, we have to be very careful about
	// clipping our mouse to them properly
	if ( AppIsTugboat() && 
		pGrid->m_pParent && 
		!pGrid->m_bIsHotBar && 
		!UIComponentCheckBounds(pGrid->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;
	y -= UIComponentGetScrollPos(pGrid).m_fY;


	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);
	float fScrollOffsetY = 0;
	if( AppIsTugboat() && pGrid->m_pParent )
	{
		x -= pGrid->m_Position.m_fX;
		logx -= pGrid->m_Position.m_fX;
		fScrollOffsetY = UIComponentGetScrollPos(pGrid->m_pParent).m_fY;
		logy += fScrollOffsetY;
	}
	int nSkillID = sSkillsGridGetSkillAtPosition(pGrid, logx, logy, skillpos);

	if (nSkillID == INVALID_LINK &&
		AppIsHellgate() )
	{
		// if we missed the skill icon, see if we're over the level up button
		
		nSkillID = sSkillsGridGetSkillLevelUpAtPosition(pGrid, logx, logy);
	}
	
	if (nSkillID == INVALID_LINK &&
		AppIsTugboat() &&
		!pGrid->m_bIsHotBar )
	{
		UI_COMPONENT * pBarPanel = UIComponentFindChildByName( pGrid->m_pParent, "skill bar panel" );

		nSkillID = sSkillsGridGetSkillBarAtPosition( pBarPanel, x + pos.m_fX, y + pos.m_fY, skillpos );

		if( nSkillID == INVALID_LINK )
		{
			pBarPanel = UIComponentFindChildByName( pGrid->m_pParent, "skill bar panel large" );

			nSkillID = sSkillsGridGetSkillBarAtPosition( pBarPanel, x + pos.m_fX, y + pos.m_fY, skillpos );
		}
	}
	skillpos.m_fY -= fScrollOffsetY;
	skillpos.m_fX *= UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	skillpos.m_fY *= UIGetScreenToLogRatioY(pGrid->m_bNoScale);

	if (nSkillID == INVALID_LINK)
	{
		// if we missed the skill icon, see if we're over the level up button
		nSkillID = sSkillsGridGetSkillShiftToggleAtPosition(pGrid, logx, logy);
		if (nSkillID != INVALID_ID)
		{
			UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pGrid);
			x += pos.m_fX;
			y += pos.m_fY;
			UISetSimpleHoverText(x, y, x, y, GlobalStringGet( GS_UI_SKILLS_PANEL_SHIFT_SKILL_TOOLTIP ));
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	if (nSkillID != INVALID_LINK)
	{
		const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLS, nSkillID);
		if (pSkillData)
		{
			UI_RECT rectIcon;
			if (!AppIsTugboat())
			{
			    rectIcon.m_fX1 = pSkillData->nSkillPageX * (pGrid->m_fCellWidth + pGrid->m_fCellBorderX);
			    rectIcon.m_fY1 = pSkillData->nSkillPageY * (pGrid->m_fCellHeight + pGrid->m_fCellBorderY);
			}
			else
            {
			    rectIcon.m_fX1 = skillpos.m_fX;
			    rectIcon.m_fY1 = skillpos.m_fY;
            }
			rectIcon.m_fX2 = rectIcon.m_fX1 + pGrid->m_fCellWidth;
			rectIcon.m_fY2 = rectIcon.m_fY1 + pGrid->m_fCellHeight;
			rectIcon.m_fX1 += pos.m_fX;
			rectIcon.m_fX2 += pos.m_fX;
			rectIcon.m_fY1 += pos.m_fY;
			rectIcon.m_fY2 += pos.m_fY;
			UISetHoverTextSkill(component, UIGetControlUnit(), nSkillID, &rectIcon);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	/*else
	{
		if( AppIsTugboat() )
		{
			int nSkillGroupID = sSkillsGridGetSlotAtPosition(pGrid, x, y, skillpos);
			if( nSkillGroupID != INVALID_ID )
			{
				const SKILLGROUP_DATA* pSkillGroupData = (SKILLGROUP_DATA *)ExcelGetData(NULL, DATATABLE_SKILLGROUPS, nSkillGroupID );
				if( !pGrid->m_bIsHotBar )
				{
					UISetSimpleHoverText( g_UI.m_Cursor, StringTableGetStringByIndex( pSkillGroupData->nDisplayString ), TRUE );					
				}
			}	
			else
			{
				UIClearHoverText();
			}
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}*/

	UIClearHoverText();

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIClearHoverText();

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);
	if (pGrid->m_nLastSkillRequestID != INVALID_LINK)
	{
		SkillStopRequest(UIGetControlUnit(), pGrid->m_nLastSkillRequestID, TRUE, TRUE);
		pGrid->m_nLastSkillRequestID = INVALID_LINK;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *sSkillsGridGetSkillItemAtPosition(
	UI_SKILLSGRID *pGrid,
	float x, 
	float y)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return NULL;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return NULL;
	}

	int nSelectedSkillID = INVALID_LINK;
	UI_GFXELEMENT *pChild = pGrid->m_pGfxElementFirst;
	while(pChild)
	{

		if (UIElementCheckBounds(pChild, x, y ) && (pChild->m_qwData)  != (QWORD)INVALID_ID && ( AppIsHellgate() || pChild->m_fZDelta != 0 ) )
		{
			nSelectedSkillID = (int)(pChild->m_qwData & 0xFFFF) ;
			break;
		}
		pChild = pChild->m_pNextElement;
	}


	if (pGrid->m_nSkillTab > INVALID_LINK)
	{
		UNIT* pItem = NULL;
		while ((pItem = UnitInventoryLocationIterate(pFocusUnit, pGrid->m_nInvLocation, pItem, 0, FALSE)) != NULL)
		{
			int nSkillID = UnitGetSkillID(pItem);
			if ( nSkillID != INVALID_ID )
			{
				const SKILL_DATA* pSkillData = SkillGetData(UnitGetGame(pFocusUnit), nSkillID);
				if (pSkillData &&
					nSkillID == nSelectedSkillID )
				{
					return pItem;
				}
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);
	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( AppIsTugboat() && !pGrid->m_bIsHotBar &&
		pGrid->m_pParent && !UIComponentCheckBounds(pGrid->m_pParent, x, y, NULL))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if( AppIsHellgate() )
	{
		y -= UIComponentGetScrollPos(pGrid).m_fY;
	}

	x -= pos.m_fX;
	y -= pos.m_fY;
	
	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);
	if( AppIsTugboat() && pGrid->m_pParent )
	{
		//logx -= pGrid->m_Position.m_fX;
		x -= pGrid->m_Position.m_fX;
		//logy += UIComponentGetScrollPos(pGrid->m_pParent).m_fY;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (!pFocusUnit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	BOOL bHasSkillPoints = (UnitGetStat(pFocusUnit, STATS_SKILL_POINTS) > 0);
	BOOL bHasPerkPoints = (UnitGetStat(pFocusUnit, STATS_PERK_POINTS) > 0);

	if ((bHasSkillPoints || bHasPerkPoints) &&
		!pGrid->m_bIsHotBar )
	{
		int nSkillID( INVALID_LINK );
		if( AppIsHellgate() )
		{
			nSkillID = sSkillsGridGetSkillLevelUpAtPosition(pGrid, logx, logy);
		}
		else
		{
			UI_POSITION skillpos;
			nSkillID = sSkillsGridGetLevelUpAtPosition( pGrid, logx, logy, skillpos );
		}
		if (nSkillID != INVALID_LINK)
		{
			const SKILL_DATA * pSkillData = SkillGetData(AppGetCltGame(), nSkillID);
			ASSERT_RETVAL(pSkillData, UIMSG_RET_NOT_HANDLED);

			BOOL bUsesPerkPoints = SkillUsesPerkPoints(pSkillData);
			if((bUsesPerkPoints && !bHasPerkPoints) || (!bUsesPerkPoints && !bHasSkillPoints))
			{
				return UIMSG_RET_NOT_HANDLED;
			}

			MSG_CCMD_REQ_SKILL_UP msg;
			msg.wSkill = (short)nSkillID;
			c_SendMessage(CCMD_REQ_SKILL_UP, &msg);

			if (AppIsTugboat())
			{
				static int nPointSpendSound = INVALID_LINK;
				if (nPointSpendSound == INVALID_LINK)
				{
					nPointSpendSound = GlobalIndexGet(  GI_SOUND_POINTSPEND );
				}
				if (nPointSpendSound != INVALID_LINK)
				{
					c_SoundPlay( nPointSpendSound );
				}
				UIClearHoverText();
			}
			else
			{
				pGrid->m_nSkillupSkill = nSkillID;
				pGrid->m_tSkillupAnimInfo.m_tiAnimStart = AppCommonGetCurTime();
				pGrid->m_tSkillupAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + 10, pGrid, UIMSG_PAINT, 0, 0);	
				pGrid->m_tSkillupAnimInfo.m_dwAnimDuration = SKILLUP_ANIM_DURATION;
				int nSoundId = GlobalIndexGet(GI_SOUND_UI_SKILLUP);
				if (nSoundId != INVALID_LINK)
				{
					c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
				}
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	int nSkillID = sSkillsGridGetSkillShiftToggleAtPosition(pGrid, logx, logy);
	if (nSkillID != INVALID_LINK)
	{

		MSG_CCMD_SKILLSHIFT_ENABLE msg;
		msg.wSkill = (short)nSkillID;
		msg.bEnabled = UnitGetStat(pFocusUnit, STATS_SKILL_SHIFT_ENABLED, nSkillID) == 0;
		c_SendMessage(CCMD_SKILLSHIFT_ENABLE, &msg);

		int nSoundId = (msg.bEnabled ? pGrid->m_nShiftToggleOnSound : pGrid->m_nShiftToggleOffSound);
		if (nSoundId != INVALID_LINK)
		{
			c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
		}
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	
	UI_POSITION posSkill;
	nSkillID = sSkillsGridGetSkillAtPosition(pGrid, logx, logy, posSkill);

	
	if (sSkillCanGoInActiveBar(nSkillID))
	{
		UISetCursorSkill(nSkillID, TRUE);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnRButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);
	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;
	float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);
	pGrid->m_nLastSkillRequestID = INVALID_LINK;

	UNIT* pPlayer = UIGetControlUnit();
	int nSkillID = sSkillsGridGetSkillAtPosition(pGrid, logx, logy, pos);
	if (nSkillID != INVALID_LINK)
	{
		if( AppIsTugboat() )
		{
			if (pGrid->m_nSkillTab != 1)
			{
				const SKILL_DATA* pSkillData = SkillGetData(AppGetCltGame(), nSkillID);

				if( pGrid->m_bIsHotBar ||
					sSkillVisibleOnPage(nSkillID, pSkillData, pGrid, pPlayer, TRUE) )
				{

					if (pSkillData->eUsage == USAGE_TOGGLE)
					{
						static int nSkillSelectSound = INVALID_LINK;
						if (nSkillSelectSound == INVALID_LINK)
						{
							nSkillSelectSound = GlobalIndexGet(  GI_SOUND_SKILL_SELECT );
						}
						if (nSkillSelectSound != INVALID_LINK)
						{
							c_SoundPlay( nSkillSelectSound );
						}
						c_SkillControlUnitRequestStartSkill( AppGetCltGame(), nSkillID );
						pGrid->m_nLastSkillRequestID = nSkillID;
					}
					else
					{



							const SKILL_DATA * pSkillData = SkillGetData( AppGetCltGame(), nSkillID );
							if ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_MOUSE ) || 
								!SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_REQUEST ))	
							{
								return UIMSG_RET_NOT_HANDLED;
							}

							if( pGrid->m_bIsHotBar )
							{
								c_SendSkillSaveAssign((WORD)nSkillID, component->m_dwParam != 0 );
							}
							else
							{
								c_SendSkillSaveAssign((WORD)nSkillID, 1 );
							}

							// this mirrors what's going on on the server (so we don't have to have it send a message back)
							UNIT *pPlayer = UIGetControlUnit();
							ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);
							int nCurrentConfig = UnitGetStat(pPlayer, STATS_CURRENT_WEAPONCONFIG);
							UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pPlayer, nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1 );
							ASSERT_RETVAL(pTag, UIMSG_RET_NOT_HANDLED);
							if (component->m_dwParam  == 0)
								pTag->m_nSkillID[0] = nSkillID;
							else
								pTag->m_nSkillID[1] = nSkillID;

							// and clear the cursor
							UISetCursorSkill(INVALID_ID, FALSE);

							if( pGrid->m_bIsHotBar )
							{
								// it also closes the hotbar immediately
								UI_COMPONENT* pParent = component->m_pParent;
								UIComponentSetVisible( pParent, FALSE );

							}

							static int nSkillSelectSound = INVALID_LINK;
							if (nSkillSelectSound == INVALID_LINK)
							{
								nSkillSelectSound = GlobalIndexGet(  GI_SOUND_SKILL_SELECT );
							}
							if (nSkillSelectSound != INVALID_LINK)
							{
								c_SoundPlay( nSkillSelectSound );
							}
							
					}
				}
				else
				{

					if ( UnitGetData( pPlayer )->m_nCantCastYetSound != INVALID_ID)
					{
						c_SoundPlay(UnitGetData( pPlayer )->m_nCantCastYetSound, &c_UnitGetPosition(pPlayer), NULL);
					}		
				}
			}			
		}
		else
		{
	//		if (SkillIsPassive(AppGetCltGame(), nSkillID))
			{
				c_SkillControlUnitRequestStartSkill( AppGetCltGame(), nSkillID );
				pGrid->m_nLastSkillRequestID = nSkillID;
			}
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);
	if (pGrid->m_nLastSkillRequestID != INVALID_LINK)
	{
		SkillStopRequest(UIGetControlUnit(), pGrid->m_nLastSkillRequestID, TRUE, TRUE);
		pGrid->m_nLastSkillRequestID = INVALID_LINK;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillsGridOnRButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SKILLSGRID *pGrid = UICastToSkillsGrid(component);
	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (pGrid->m_nLastSkillRequestID != INVALID_LINK)
	{
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
			SkillStopRequest(pPlayer, pGrid->m_nLastSkillRequestID, TRUE, TRUE);
		pGrid->m_nLastSkillRequestID = INVALID_LINK;
	}


	return UIMSG_RET_HANDLED;
}

//---------------------------------------------------------------------------- 
//----------------------------------------------------------------------------
UNIT * sInvGridGetItemAtCursorPos(
	UI_INVGRID *pGrid,
	float x,
	float y,
	int *invx = NULL,
	int *invy = NULL)
{
	UNIT* container = UIComponentGetFocusUnit(pGrid);
	if (!container)
	{
		return NULL;
	}

	int location = pGrid->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		return NULL;
	}

	UI_POSITION pos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return NULL;
	}

	pos.m_fX -= UIComponentGetScrollPos(pGrid).m_fX;
	pos.m_fY -= UIComponentGetScrollPos(pGrid).m_fY;

	int nInvx = (int)((x - pos.m_fX) / UIGetScreenToLogRatioX(pGrid->m_bNoScale) / (pGrid->m_fCellWidth + pGrid->m_fCellBorder));
	int nInvy = (int)((y - pos.m_fY) / UIGetScreenToLogRatioY(pGrid->m_bNoScale) / (pGrid->m_fCellHeight + pGrid->m_fCellBorder));

	if (invx)
	{
		*invx = nInvx;
	}

	if (invy)
	{
		*invy = nInvy;
	}

	if (nInvx < 0 || nInvx > info.nWidth - 1 ||
		nInvy < 0 || nInvy > info.nHeight - 1)
	{
		return NULL;
	}

	return UnitInventoryGetByLocationAndXY(container, location, nInvx, nInvy);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	UNIT* container = UnitGetById(pGame, (UNITID)wParam);
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (UnitGetId(container) != UIComponentGetFocusUnitId(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVGRID* invgrid = UICastToInvGrid(component);

	int location = (int)lParam;
	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	if (location != -1 && location != invgrid->m_nInvLocation && location != nCursorLocation)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnMouseMove(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_INVGRID* invgrid = UICastToInvGrid(component);
	if (UIComponentGetActive(component) && UIComponentCheckBounds(invgrid))
	{
		float x, y;

		UIGetCursorPosition(&x, &y);
		UNIT* item = sInvGridGetItemAtCursorPos(invgrid, x, y);
		if (!item ||
			UIGetHoverUnit() != UnitGetId(item))
		{
			UISetHoverUnit(INVALID_ID, UIComponentGetId(invgrid));
		}

		UIInvGridOnInventoryChange(invgrid, UIMSG_INVENTORYCHANGE, UIComponentGetFocusUnitId(invgrid), invgrid->m_nInvLocation);

		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVGRID* invgrid = UICastToInvGrid(component);

	UI_POSITION pos;
	if (UIComponentCheckBounds(component, x, y, &pos))
	{
		int invx = -1, invy = -1;
		UNIT* item = sInvGridGetItemAtCursorPos(invgrid, x, y, &invx, &invy);
		if (!item)
		{
			UISetHoverUnit(INVALID_ID, UIComponentGetId(invgrid));
			return UIMSG_RET_HANDLED;
		}

		float fScale = 1.0f;
		if (invx >= 0 && invx < invgrid->m_nGridWidth && invy >= 0 && invy < invgrid->m_nGridHeight)
		{
			int iScale = (invy * invgrid->m_nGridWidth) + invx;
			fScale = invgrid->m_pItemAdj[iScale].fScale;
		}

		float w = invgrid->m_fCellWidth + invgrid->m_fCellBorder;
		float h = invgrid->m_fCellHeight + invgrid->m_fCellBorder;

			UISetHoverTextItem(
				&UI_RECT(pos.m_fX + invx * w, 
						 pos.m_fY + invy * h, 
						 pos.m_fX + ( (invx + 1) * w * UIGetScreenToLogRatioX( invgrid->m_bNoScale ) ),
						 pos.m_fY + ( (invy + 1) * h * UIGetScreenToLogRatioY( invgrid->m_bNoScale )  ) ), 
				item,
				fScale);

		UISetHoverUnit(UnitGetId(item), UIComponentGetId(invgrid));

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVGRID* invgrid = UICastToInvGrid(component);

	UIInvGridOnInventoryChange(invgrid, UIMSG_INVENTORYCHANGE, UIComponentGetFocusUnitId(invgrid), invgrid->m_nInvLocation);
	UISetHoverUnit(INVALID_ID, UIComponentGetId(invgrid));

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UI_TB_ItemUse(UNIT *pItem)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackConfirmSell( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	UNIT *pItem = (UNIT *)pUserData;
	GAME *game = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit(game);
	UNIT *pMerchant = TradeGetTradingWith( pPlayer );

	if (pMerchant && c_ItemSell(game, pPlayer, pMerchant, pItem))
	{
		static int nSellSound = INVALID_LINK;
		if (nSellSound == INVALID_LINK)
		{
			nSellSound = GlobalIndexGet( GI_SOUND_ITEM_PURCHASE );
		}
		if (nSellSound != INVALID_LINK)
		{
			c_SoundPlay(nSellSound, &c_SoundGetListenerPosition(), NULL);
		}
	}
}

BOOL UIDoMerchantAction(
	UNIT *pItem,
	int nSuggestedLocation, 
	int nSuggestedX,
	int nSuggestedY)
{
	if (UIIsMerchantScreenUp() )
	{
		//UI_COMPONENT* merchant_screen = UIComponentGetByEnum(UICOMP_MERCHANTINVENTORY);
		//UNIT *pMerchant = UIComponentGetFocusUnit(merchant_screen);
		UNIT *pMerchant = NULL;
		if (ItemBelongsToAnyMerchant( pItem ))
		{
			pMerchant = ItemGetMerchant(pItem);
		}
		else
		{
			UNIT *pPlayer = UnitGetUltimateContainer( pItem );
			ASSERTX_RETFALSE( pPlayer, "Expected unit" );
			ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
			pMerchant = TradeGetTradingWith( pPlayer );
		}
		
		if (pMerchant)
		{
			// First we should close the mod/item exam panel if it's open for this item
			UI_COMPONENT* pGunModComponent = UIComponentGetByEnum(UICOMP_GUNMODSCREEN);
			if( pGunModComponent )
			{
				UI_GUNMOD *pGunMod = UICastToGunMod( pGunModComponent );
				if (pGunMod && UIComponentGetActive(pGunMod))
				{
					UNIT* pModItem = UnitGetById(AppGetCltGame(), UIGetModUnit());
					if (pModItem == pItem)
					{
						UIComponentActivate(pGunMod, FALSE);
					}
				}
			}

			ASSERT_RETFALSE(UnitIsMerchant(pMerchant));
			ASSERT_RETFALSE(UnitIsA(pItem, UNITTYPE_ITEM));
			UNIT *pPlayer = UIGetControlUnit();

			GAME *game = AppGetCltGame();
			static int nSellSound = INVALID_LINK;
			if (nSellSound == INVALID_LINK)
			{
				nSellSound = GlobalIndexGet( GI_SOUND_ITEM_PURCHASE );
			}

			// Selling to merchant
			if (ItemBelongsToAnyMerchant( pItem ) == FALSE)
			{
				const UNIT_DATA *pItemData = UnitGetData( pItem );
				if (!UnitDataTestFlag(pItemData, UNIT_DATA_FLAG_IGNORE_SELL_WITH_INVENTORY_CONFIRM))
				{
					// Tugboat just lets you sell socketed items for now
					if (AppIsHellgate() &&
						!UnitIsA(pItem, UNITTYPE_SINGLE_USE_RECIPE)
						&& UnitInventoryIterate(pItem, NULL) != NULL)
					{
						DIALOG_CALLBACK tCallbackOK;
						DialogCallbackInit( tCallbackOK );
						tCallbackOK.pfnCallback = sCallbackConfirmSell;
						tCallbackOK.pCallbackData = pItem;
						UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_ALERT ), GlobalStringGet( GS_CONFIRM_SELL_WITH_INVENTORY ), TRUE, &tCallbackOK );
						return TRUE;
					}
				}
				if (c_ItemSell(game, pPlayer, pMerchant, pItem))
				{
					if (nSellSound != INVALID_LINK)
					{
						c_SoundPlay(nSellSound, &c_SoundGetListenerPosition(), NULL);
					}
					return TRUE;
				}
			}
			else
			{
				if (c_ItemBuy(game, pPlayer, pMerchant, pItem, nSuggestedLocation, nSuggestedX, nSuggestedY))
				{
					if (nSellSound != INVALID_LINK)
					{
						c_SoundPlay(nSellSound, &c_SoundGetListenerPosition(), NULL);
					}
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIDoMerchantAction(UNIT * pItem)
{
	return UIDoMerchantAction(pItem, INVLOC_NONE, -1, -1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTryButtonDownItemModes(
	UNIT *pItem)
{	

	// if in identify state, try to id (it's ok if item is NULL, that will cancel the state)	
    if (UIGetCursorState() == UICURSOR_STATE_IDENTIFY)
    {
		c_ItemTryIdentify( pItem, TRUE );
	    return TRUE;
    }

	// if the item needs to pick another item before executing it's skill
	if( pItem && UIGetCursorState() == UICURSOR_STATE_ITEM_PICK_ITEM )
	{
		c_ItemPickItemExecuteSkill( pItem, UnitGetById( UnitGetGame( pItem ), UIGetCursorUseUnit() ) );
	    return TRUE;
	}

	// if in dismantle state, try to do it (it's ok if item is NULL, that will cancel the state)	
    if (UIGetCursorState() == UICURSOR_STATE_DISMANTLE)
    {
		c_ItemTryDismantle( pItem, TRUE );
	    return TRUE;
    }

	return FALSE;  // no modes to try
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIInvGridOnLButtonDownNoCursorUnit(
	UI_INVGRID* invgrid,
	UI_POSITION pos,
	float fCursX, 
	float fCursY)
{
	UNIT* container = UIComponentGetFocusUnit(invgrid);
	if (!container)
	{
		return UIMSG_RET_END_PROCESS;
	}

	int location = invgrid->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		return UIMSG_RET_END_PROCESS;
	}

	// compensate for scrolling
	pos.m_fX -= UIComponentGetScrollPos(invgrid).m_fX;
	pos.m_fY -= UIComponentGetScrollPos(invgrid).m_fY;

	int invx = (int)((fCursX - pos.m_fX) / UIGetScreenToLogRatioX(invgrid->m_bNoScale) / (invgrid->m_fCellWidth + invgrid->m_fCellBorder));
	invx = PIN(invx, 0, info.nWidth - 1);
	int invy = (int)((fCursY - pos.m_fY) / UIGetScreenToLogRatioY(invgrid->m_bNoScale) / (invgrid->m_fCellHeight + invgrid->m_fCellBorder));
	invy = PIN(invy, 0, info.nHeight - 1);

	UNIT* item = UnitInventoryGetByLocationAndXY(container, location, invx, invy);

	// handle item modes
	if (sTryButtonDownItemModes( item ) == TRUE)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	// no item, don't continue	
	if (!item)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (AppIsTugboat())
	{
		// items can be shift-left-clicked to be sold/bought
		if ( UIIsMerchantScreenUp() &&
			(GetKeyState(VK_SHIFT) & 0x8000)	)
		{
			UIDoMerchantAction(item);

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		
	}
	// if the item is in a trade slot that is not ours, we can only look, but not touch
	UNIT *pUltimateContainer = UnitGetUltimateContainer( item );
	UNIT *pPlayer = GameGetControlUnit( AppGetCltGame() );
	if (InventoryIsInTradeLocation( item ))
	{
		if (pPlayer != pUltimateContainer)
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

    if (UIGetCursorState() == UICURSOR_STATE_SKILL_PICK_ITEM)			
    {
	    if (pPlayer == pUltimateContainer)
	    {
		    ASSERT_RETVAL(g_UI.m_Cursor, UIMSG_RET_NOT_HANDLED);
			if(SkillIsValidTarget(AppGetCltGame(), pPlayer, item, NULL, g_UI.m_Cursor->m_nItemPickActivateSkillID, FALSE))
			{
				c_SkillControlUnitRequestStartSkill(AppGetCltGame(), g_UI.m_Cursor->m_nItemPickActivateSkillID, item);
			}
	    }

	    return UIMSG_RET_HANDLED_END_PROCESS;
    }

	// this is commented out until the database operations can be made atomic to prevent duping.
	// KCK: The database issue has been fixed, but this feature has been replaced by the
	// stack splitting dialog/message.
	//if (!AppIsTugboat())
	//{
	//	// items can be ctrl-left-clicked to take one item off of the stack
	//	if (GetKeyState(VK_CONTROL) & 0x8000)	
	//	{
	//		if ( pPlayer == pUltimateContainer )
	//		{
	//			MSG_CCMD_ITEM_SPLIT_STACK msg;
	//			msg.idItem = UnitGetId(item);
	//			msg.nStackPrevious = ItemGetQuantity(item);
	//			msg.nNewStackSize = 1;
	//			c_SendMessage(CCMD_ITEM_SPLIT_STACK, &msg);

	//			UIPlayItemPutdownSound(item, pPlayer, GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR));
	//		}

	//		return UIMSG_RET_HANDLED_END_PROCESS;
	//	}
	//}


    // get the unit's upper-left corner (not the grid square the mouse is over) to calculate the offset
    int nLoc, nItemX, nItemY;
    UnitGetInventoryLocation(item, &nLoc, &nItemX, &nItemY);

	if (nLoc == INVLOC_SHARED_STASH)
	{
		if (!InventoryLocPlayerCanTake(pPlayer, item, nLoc))
		{
			int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
			if (nSoundId != INVALID_LINK)
			{
				c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

    float fOffsetX = ((float)nItemX * (invgrid->m_fCellWidth + invgrid->m_fCellBorder)) - (fCursX - pos.m_fX);
    float fOffsetY = ((float)nItemY * (invgrid->m_fCellHeight + invgrid->m_fCellBorder)) - (fCursY - pos.m_fY);

    UI_SIZE sizeItem = sGetItemSizeInGrid(item, invgrid, &info);
    float fOffsetPctX = fOffsetX / sizeItem.m_fWidth;
    float fOffsetPctY = fOffsetY / sizeItem.m_fHeight;
    if (UISetCursorUnit(UnitGetId(item), TRUE, -1, fOffsetPctX, fOffsetPctY))
	{
		UIPlayItemPickupSound(item);
		UIComponentHandleUIMessage(invgrid, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIInvGridOnLButtonDownWithCursorUnit(
	UI_INVGRID* invgrid,
	UI_POSITION pos,
	float fCursX, 
	float fCursY)
{
	UNITID idCursorUnit = UIGetCursorUnit();
	UNIT* unitCursor = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!unitCursor)
	{
		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_END_PROCESS;
	}

	UNIT* pUltimateContainer = UnitGetUltimateContainer(unitCursor);

	BOOL bInMultipleConfigs = ( pUltimateContainer && UnitIsA(pUltimateContainer, UNITTYPE_PLAYER) ) ? ItemInMultipleWeaponConfigs(unitCursor) : FALSE;
	if (UICursorUnitIsReference())
	{
		// if you only have a reference in the cursor don't move anything, just clear the cursor...
		// this may have to be set to the play backpack only.  I believe cursor item references are
		// mainly used for weaponconfigs anymore.

		if (bInMultipleConfigs &&
			g_UI.m_Cursor->m_nFromWeaponConfig != -1 &&
			g_UI.m_Cursor->m_nLastInvLocation == INVLOC_NONE)
		{
			// Also clear it out of the weapon config.
			MSG_CCMD_REMOVE_FROM_WEAPONCONFIG msg;
			msg.idItem = idCursorUnit;
			msg.nWeaponConfig = g_UI.m_Cursor->m_nFromWeaponConfig;
			c_SendMessage(CCMD_REMOVE_FROM_WEAPONCONFIG, &msg);
		}

		if (bInMultipleConfigs ||
			g_UI.m_Cursor->m_nLastInvLocation != INVLOC_NONE)
		{
			UIClearCursorUnitReferenceOnly();

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	UNIT* container = UIComponentGetFocusUnit(invgrid);
	if (!container)
	{
		UIClearCursor();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	// did we click on a merchant inventory location
	BOOL bClickedOnMerchantGrid = InvLocIsStoreLocation(container, invgrid->m_nInvLocation);
	
	// don't allow dropping an item on a reward grid.
	if (!bClickedOnMerchantGrid && !InventoryLocPlayerCanPut(container, unitCursor, invgrid->m_nInvLocation))
	{
#if !ISVERSION(SERVER_VERSION)
		if(invgrid->m_nInvLocation == INVLOC_SHARED_STASH && !PlayerIsSubscriber(container))
		{
			const WCHAR *szMsg = GlobalStringGet(GS_SUBSCRIBER_ONLY_CHARACTER_HEADER);
			if (szMsg && szMsg[0])
			{
				UIShowQuickMessage(szMsg, 3.0f);
			}
			int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
			if (nSoundId != INVALID_LINK)
			{
				c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
			}
		}
#endif
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if( AppIsTugboat() )
	{
		// hide any confirmations we've got up - we just dropped something!
		UIHideGenericDialog();
	}

	static int nSellSound = INVALID_LINK;
	if (nSellSound == INVALID_LINK)
	{
		nSellSound = GlobalIndexGet( GI_SOUND_SELL );
	}

	int location = invgrid->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		UIClearCursor();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	// compensate for scrolling
	pos.m_fX -= UIComponentGetScrollPos(invgrid).m_fX;
	pos.m_fY -= UIComponentGetScrollPos(invgrid).m_fY;

	if (!AppIsTugboat() && unitCursor)
	{
		// make the apparent cursor position the top-left of the item.
		ASSERT(g_UI.m_Cursor);
		fCursX += g_UI.m_Cursor->m_tItemAdjust.fXOffset + (invgrid->m_fCellWidth / 2.0f);
		fCursY += g_UI.m_Cursor->m_tItemAdjust.fYOffset + (invgrid->m_fCellHeight / 2.0f);
	}

	int invx = (int)((fCursX - pos.m_fX) / UIGetScreenToLogRatioX(invgrid->m_bNoScale) / (invgrid->m_fCellWidth + invgrid->m_fCellBorder));
	invx = PIN(invx, 0, info.nWidth - 1);
	int invy = (int)((fCursY - pos.m_fY) / UIGetScreenToLogRatioY(invgrid->m_bNoScale) / (invgrid->m_fCellHeight + invgrid->m_fCellBorder));
	invy = PIN(invy, 0, info.nHeight - 1);

	UNIT* item = NULL;
	int nFromWeaponConfig = g_UI.m_Cursor->m_nFromWeaponConfig;

    if (!bClickedOnMerchantGrid &&
		InventoryGridGetItemsOverlap(container, unitCursor, item, location, invx, invy) > 1)
    {
	    // ain't gonna fit in there
	    return UIMSG_RET_HANDLED_END_PROCESS;
	}    

	// try button down item modes
	if (sTryButtonDownItemModes( item ) == TRUE)
	{
	    return UIMSG_RET_HANDLED_END_PROCESS;
    }

	// check for manipulating items in a store
	if (ItemBelongsToAnyMerchant(unitCursor))
	{
		BOOL bBought = FALSE;
		if (bClickedOnMerchantGrid)
		{
			// put will be item back into the store		
			goto putdown;
		}		
		else
		{
			// moving from merchant inventory to player inventory		
			bBought = UIDoMerchantAction(unitCursor, location, invx, invy);
			if (bBought)
			{
			    UISetCursorUnit(INVALID_ID, TRUE);
			}
		}
		
		if (bBought && UnitIsA(unitCursor, UNITTYPE_MOD) && UnitGetInventoryType(item) > 0)
		{
			// fall through and place the mod
		}
		else
		{
			// no more processing to do
			return UIMSG_RET_HANDLED_END_PROCESS;						
		}
		
	}
	else
	{
		if (bClickedOnMerchantGrid)
		{
			// giving a player owned item to a merchant
			if (UIDoMerchantAction(unitCursor))
			{
				// TRAVIS - in Mythos, this doesn't seem to play nice
				// with merchant buyback, as it can throw it immediately back into
				// your own inventory before the sell completes?

				// BSP - yep, seems so.  Not sure what changed
				//if( AppIsHellgate() )
				//{
				//	UISetCursorUnit(INVALID_ID, TRUE);
				//}
			}
			return UIMSG_RET_HANDLED_END_PROCESS;			
		}
	}
	
	if (!item || item == unitCursor)
	{
		if (bClickedOnMerchantGrid)
		{
			goto putdown;
		}

		if (InventoryTestPut( container, unitCursor, location, invx, invy, 0 ))
		{
			c_InventoryTryPut( container, unitCursor, location, invx, invy );
			UIClearCursorUnitReferenceOnly();
			UnitWaitingForInventoryMove(unitCursor, TRUE);
			goto putdown;
		}
		else
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	if (item == unitCursor)
	{
		goto putdown;
	}


//	if (!UIIsMerchantScreenUp())
	{
		BOOL bCanReplace = ItemIsACraftedItem( item ) && UnitIsA( unitCursor, UNITTYPE_CRAFTING_MOD );

		// if cursor is a mod, and item clicked on can have an inventory...
		if (UnitIsA(unitCursor, UNITTYPE_MOD) && 
			UnitGetInventoryType(item) > 0 &&
			( bCanReplace || ItemGetOpenLocation(item, unitCursor, TRUE, NULL, NULL, NULL) ) )
		{
			if( AppIsTugboat() )
			{
				if( sUISocketedItemOnLButtonDownWithCursorUnit( item ) )
				{
					goto putdown;
				}
				else
				{
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
			else if (!ItemIsInSharedStash(item))
			{
				UIPlaceMod(item, unitCursor, !UIIsGunModScreenUp());
				goto putdown;
			}

		}
	}

	// check if a swap is possible
	if (!UnitInventoryTestSwap(item, unitCursor, invx, invy))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (location == INVLOC_SHARED_STASH)
	{
		if (!InventoryLocPlayerCanTake(container, item, location))
		{
			int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
			if (nSoundId != INVALID_LINK)
			{
				c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	//special case: we want this to look like a normal swap.  Put the grid item in the cursor
	//  and move the WC item into the grid.
	if (nFromWeaponConfig != -1 &&
		!bInMultipleConfigs)
	{
		// move the grid item to the cursor
		UISetCursorUnit(UnitGetId(item), FALSE, -1);

		// move the WC item to the grid
		c_InventoryTryPut(container, unitCursor, location, invx, invy);
	}
	else
	{
		c_InventoryTrySwap(unitCursor, item, nFromWeaponConfig, invx, invy);
	}

	if (!AppIsTugboat())
    {
		g_UI.m_Cursor->m_tItemAdjust.fScale = 1.5f;
		UIGetStdItemCursorSize(item, g_UI.m_Cursor->m_fItemDrawWidth, g_UI.m_Cursor->m_fItemDrawHeight);

		g_UI.m_Cursor->m_tItemAdjust.fXOffset = -0.5f * g_UI.m_Cursor->m_fItemDrawWidth;
		g_UI.m_Cursor->m_tItemAdjust.fYOffset = -0.5f * g_UI.m_Cursor->m_fItemDrawHeight;
    }

putdown:
	if (bClickedOnMerchantGrid || 
		(nFromWeaponConfig != -1 && bInMultipleConfigs))
	{
		UIClearCursor();
	}		

	UIPlayItemPutdownSound(unitCursor, container, location);
	if (!AppIsTugboat() &&
		invx >= 0 && invx < invgrid->m_nGridWidth && invy >= 0 && invy < invgrid->m_nGridHeight)
	{
		int iGridsquare = (invy * invgrid->m_nGridWidth) + invx;
		ASSERT(g_UI.m_Cursor);
		invgrid->m_pItemAdj[iGridsquare].fXOffset = (fCursX - pos.m_fX - (invgrid->m_fCellWidth / 2.0f)) - ((float)invx * (invgrid->m_fCellWidth + invgrid->m_fCellBorder));
		invgrid->m_pItemAdj[iGridsquare].fYOffset = (fCursY - pos.m_fY - (invgrid->m_fCellWidth / 2.0f)) - ((float)invy * (invgrid->m_fCellHeight + invgrid->m_fCellBorder));
		invgrid->m_pItemAdj[iGridsquare].fScale = g_UI.m_Cursor->m_tItemAdjust.fScale;

		invgrid->m_pItemAdjStart[iGridsquare] = invgrid->m_pItemAdj[iGridsquare];

		if (invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket);
		}

		invgrid->m_tItemAdjAnimInfo.m_tiAnimStart = AppCommonGetCurTime();
		invgrid->m_tItemAdjAnimInfo.m_dwAnimDuration = 150;
		invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIInvGridDeflate, CEVENT_DATA((DWORD_PTR)invgrid, 0));	
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float fCursX, fCursY;
	UIGetCursorPosition(&fCursX, &fCursY);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, fCursX, fCursY, &pos) || !UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UIGetCursorSkill() != INVALID_ID)
		return UIMSG_RET_HANDLED_END_PROCESS;

	UI_INVGRID* invgrid = UICastToInvGrid(component);
	UI_MSG_RETVAL eResult;
	
	if (UIGetCursorUnit() == INVALID_ID)
	{
		eResult = sUIInvGridOnLButtonDownNoCursorUnit(invgrid, pos, fCursX, fCursY);
	}
	else
	{
		eResult = sUIInvGridOnLButtonDownWithCursorUnit(invgrid, pos, fCursX, fCursY);
	}
	
	return eResult;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInvLocTryPut(
	UNIT *container,
	UNIT *item,
	int nLoc)
{
	if (nLoc == INVALID_LINK)
	{
		return FALSE;
	}

	int x, y;
	if (UnitInventoryFindSpace( container, item, nLoc, &x, &y, 0 ))
	{
		c_InventoryTryPut( container, item, nLoc, x, y );
		UnitWaitingForInventoryMove(item, TRUE);
		UIPlayItemPutdownSound(item, container, nLoc);
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnRButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{


	if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVGRID* invgrid = UICastToInvGrid(component);
	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(invgrid, x, y, &pos) || !UIComponentGetVisible(invgrid))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UIGetCursorSkill() != INVALID_ID)
		return UIMSG_RET_HANDLED_END_PROCESS;

	if (UIGetCursorUnit() != INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* container = UIComponentGetFocusUnit(invgrid);
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int location = invgrid->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		UIClearCursor();
		return UIMSG_RET_NOT_HANDLED;
	}

	// compensate for scrolling
	pos.m_fX -= UIComponentGetScrollPos(invgrid).m_fX;
	pos.m_fY -= UIComponentGetScrollPos(invgrid).m_fY;

	int invx = (int)((x - pos.m_fX) / UIGetScreenToLogRatioX(invgrid->m_bNoScale) / (invgrid->m_fCellWidth + invgrid->m_fCellBorder));
	invx = PIN(invx, 0, info.nWidth - 1);
	int invy = (int)((y - pos.m_fY) / UIGetScreenToLogRatioY(invgrid->m_bNoScale) / (invgrid->m_fCellHeight + invgrid->m_fCellBorder));
	invy = PIN(invy, 0, info.nHeight - 1);

	UNIT* item = UnitInventoryGetByLocationAndXY(container, location, invx, invy);
	if (!item)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (AppIsTugboat())
	{
		// right-click use items must be dragged or shift-left-clicked to be sold
		/*if ( UIIsMerchantScreenUp() &&
			!( ( ItemHasSkillWhenUsed( item ) && UnitIsA( item, UNITTYPE_ANALYZER ) ) && !ItemBelongsToAnyMerchant(item) ) &&
			!UnitInventoryIterate(item, NULL))	// if the item does not contain any other items (mods)
		{
			if (UIDoMerchantAction(item))
			{
				return UIMSG_RET_HANDLED_END_PROCESS;
			}
		}
		else*/
		{
			if (ItemHasSkillWhenUsed( item ))
			{
				if( UnitIsUsable( UIGetControlUnit(), item ) )
				{
					c_ItemUse(item);
				}
				else
				{
					UIPlayCantUseSound( AppGetCltGame(), UIGetControlUnit(), item );
				}
			}
			else if( AppIsHellgate() )
			{
				if (UnitIsUsable(UIGetControlUnit(), item) == USE_RESULT_NOT_USABLE &&
					!UnitIsA(item, UNITTYPE_CONSUMABLE))
				{
					UIPlaceMod(item, NULL);
				}
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	else
	{
		if (UIIsMerchantScreenUp() &&
			(GetKeyState(VK_SHIFT) & 0x8000)	&&	// the shift key is down
			(UnitIsA(item, UNITTYPE_SINGLE_USE_RECIPE) || !UnitInventoryIterate(item, NULL)))	// if the item does not contain any other items (mods)
		{
			// buy or sell the item
			if (UIDoMerchantAction(item))
			{
				return UIMSG_RET_HANDLED_END_PROCESS;
			}
		}
		if (UIIsStashScreenUp() &&
			(GetKeyState(VK_SHIFT) & 0x8000))		// the shift key is down
		{
			if (InvLocIsStashLocation(container, location))
			{
				// try to put the item in your backpack
				sInvLocTryPut( container, item, GlobalIndexGet( GI_INVENTORY_LOCATION_BIGPACK) );
			}
			else if (UIComponentGetVisibleByEnum(UICOMP_SHARED_STASH_TAB))
			{
				// try to put the item in your shared stash
				sInvLocTryPut( container, item, GlobalIndexGet( GI_INVENTORY_LOCATION_SHARED_STASH ));
			}
			else
			{
				// try to put the item in your personal stash
				if (!sInvLocTryPut( container, item, GlobalIndexGet( GI_INVENTORY_LOCATION_STASH) ))
				{
					if (PlayerIsSubscriber(container))
					{
						sInvLocTryPut( container, item, GlobalIndexGet( GI_INVENTORY_LOCATION_EXTRA_STASH) );
					}
				}
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		else
		{

			c_InteractRequestChoices(item, component);

			//// We're not going to the server to get the possible choices right now because the client
			//// knows all it needs to about the items to display the radial menu.
		
			//INTERACT_MENU_CHOICE ptInteractChoices[MAX_INTERACT_CHOICES];
			//int nChoicesCount = 0;
			//ItemGetInteractChoices(UIGetControlUnit(), item, ptInteractChoices, nChoicesCount);
			//c_InteractDisplayChoices(UIGetControlUnit(), item, ptInteractChoices, nChoicesCount );
		}

	}

	return UIMSG_RET_HANDLED;			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnRButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_INVGRID* invgrid = UICastToInvGrid(component);
	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(invgrid, x, y, &pos) || !UIComponentGetActive(invgrid))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UIGetCursorUnit() != INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* container = UIComponentGetFocusUnit(invgrid);
	if (container != UIGetControlUnit())
	{
		return UIMSG_RET_NOT_HANDLED;
	}


	int location = invgrid->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		UIClearCursor();
		return UIMSG_RET_NOT_HANDLED;
	}

	int invx = (int)((x - pos.m_fX) / UIGetScreenToLogRatioX(invgrid->m_bNoScale) / (invgrid->m_fCellWidth + invgrid->m_fCellBorder));
	invx = PIN(invx, 0, info.nWidth - 1);
	int invy = (int)((y - pos.m_fY) / UIGetScreenToLogRatioY(invgrid->m_bNoScale) / (invgrid->m_fCellHeight + invgrid->m_fCellBorder));
	invy = PIN(invy, 0, info.nHeight - 1);

	UNIT* item = UnitInventoryGetByLocationAndXY(container, location, invx, invy);
	if (!item)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (ItemBelongsToAnyMerchant( item ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	
	if (!AppIsTugboat())
    {
	    USE_RESULT eUseResult = UnitIsUsable( UIGetControlUnit(), item );
	    if (eUseResult == USE_RESULT_OK)
	    {
		    c_ItemUse(item);
	    }
	    else if (eUseResult != USE_RESULT_NOT_USABLE)
	    {
		    c_ItemCannotUse( item, eUseResult );
	    }
    }
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvGridOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pRadialMenu = UIComponentGetByEnum(UICOMP_RADIAL_MENU);

	if (pRadialMenu)
	{
		UIComponentSetVisible(pRadialMenu, FALSE);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMerchantInvGridOnSetControlUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_INVGRID* invgrid = UICastToInvGrid(component);

	// go back to default location
	invgrid->m_nInvLocation = invgrid->m_nInvLocDefault;

	// get player
	UNIT *pPlayer = GameGetControlUnit(AppGetCltGame());
	if (pPlayer)
	{
		UNIT *pFocus = UIComponentGetFocusUnit(component);
		
		if (pFocus)
		{
			if (pFocus == pPlayer)
			{
				if (invgrid->m_nInvLocPlayer != INVALID_LINK)
				{
					invgrid->m_nInvLocation = invgrid->m_nInvLocPlayer;
				}
			}
			else
			{
			
				// priority given to merchant slot if present ... oh how i extend this hack ;)
				if ( UnitIsMerchant(pFocus) && invgrid->m_nInvLocMerchant != INVALID_LINK)
				{
					invgrid->m_nInvLocation = invgrid->m_nInvLocMerchant;
				}
				
				// the grid this merchant shows will be dependent on the player class ... ick 
				else if (UnitIsA(pPlayer, UNITTYPE_CABALIST) && invgrid->m_nInvLocCabalist != INVALID_LINK)
				{
					invgrid->m_nInvLocation = invgrid->m_nInvLocCabalist;
				}
				else if (UnitIsA(pPlayer, UNITTYPE_HUNTER) && invgrid->m_nInvLocHunter != INVALID_LINK)
				{
					invgrid->m_nInvLocation = invgrid->m_nInvLocHunter;
				}
				else if (UnitIsA(pPlayer, UNITTYPE_TEMPLAR) && invgrid->m_nInvLocTemplar != INVALID_LINK)
				{
					invgrid->m_nInvLocation = invgrid->m_nInvLocTemplar;
				}				
			}
		}
	}

	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT* container = UnitGetById(AppGetCltGame(), (UNITID)wParam);
	if (!container)
	{
		// we seem to have gotten a message late, the unit is already gone
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT *pUnitFocus = UIComponentGetFocusUnit( component );
	if (pUnitFocus == NULL)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	
	// note that we want to check ultimate container here so that we can catch the player
	if (UnitGetUltimateContainer( container ) != UnitGetUltimateContainer( pUnitFocus ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVSLOT* invslot = UICastToInvSlot(component);

	int location = (int)lParam;
	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	if (location != -1 && location != invslot->m_nInvLocation && location != nCursorLocation)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIInvslotDeflate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_INVSLOT *invslot = (UI_INVSLOT *)data.m_Data1;
	ASSERT_RETURN(invslot);

	BOOL bFromPaint = (BOOL)data.m_Data2;

	if (invslot->m_tItemAdjAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(invslot->m_tItemAdjAnimInfo.m_dwAnimTicket);
	}

	TIME tiCurrentTime = AppCommonGetCurTime();
	if (sIncItemAdjust(invslot->m_ItemAdj, invslot->m_ItemAdjStart, tiCurrentTime, invslot->m_tItemAdjAnimInfo))
	{
		// schedule the next one
		invslot->m_tItemAdjAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIInvslotDeflate, CEVENT_DATA((DWORD_PTR)invslot, 0));	
	}

	if (!bFromPaint)
	{
		UIComponentHandleUIMessage(invslot, UIMSG_PAINT, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIInvSlotSetUnitId(
	UI_INVSLOT* invslot,
	const UNITID unitid)
{
	ASSERT_RETURN(invslot);
	invslot->m_nInvUnitId = unitid;
	invslot->m_bUseUnitId = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT* sUIInvSlotGetItem(
	UNIT* container,
	UI_INVSLOT* invslot)
{
	ASSERT_RETNULL(container);
	ASSERT_RETNULL(invslot);

	if (invslot->m_bUseUnitId)
	{
		return UnitInventoryGetByLocationAndUnitId(container, invslot->m_nInvLocation, invslot->m_nInvUnitId);
	}
	else
	{
		return UnitInventoryGetByLocation(container, invslot->m_nInvLocation);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame || AppGetState() != APP_STATE_IN_GAME)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVSLOT* invslot = UICastToInvSlot(component);
		
	UI_COMPONENT* pSlotIcon = UIComponentFindChildByName(component, "slot icon");
	UI_COMPONENT* pOverlayComponent = UIComponentFindChildByName(component, "overlay");
	UI_COMPONENT* pLitFrameComponent = UIComponentFindChildByName(component, "sublitframe");

	if( !pLitFrameComponent )
	{
		pLitFrameComponent = component;
	}

	if (pOverlayComponent)
		UIComponentRemoveAllElements(pOverlayComponent);
	// Travis: We sort of still need this
	if( AppIsTugboat() )
	{
		UIComponentRemoveAllElements(component);
	}

	UNIT* item = NULL;
	UNIT* container = NULL;
	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		container = UIGetControlUnit();
		item = UIComponentGetFocusUnit(invslot);
	}
	else
	{ 
		container = UIComponentGetFocusUnit(invslot);
		if( container )
		{
			item = sUIInvSlotGetItem(container, invslot);
			const INVLOC_DATA *pInvLocData = UnitGetInvLocData( container, invslot->m_nInvLocation );
			if (pInvLocData == NULL)
			{
				// this unit doesn't have this slot
				return UIMSG_RET_HANDLED;
			}
		}
	}

	if (!container)
	{
		// we seem to have gotten a message late, the unit is already gone
		return UIMSG_RET_NOT_HANDLED;
	}

	//BOOL bIsMerchant = UnitIsMerchant(container);
	BOOL bIsMerchant = InvLocIsStoreLocation(container, invslot->m_nInvLocation);
	UI_COMPONENT *pChild = UIComponentFindChildByName(invslot, "invslot price text");
	UI_LABELEX *pPriceText = (pChild && UIIsMerchantScreenUp() ? UICastToLabel(pChild) : NULL);
	if (pPriceText)
	{
		UIComponentRemoveAllElements(pPriceText);
	}
	pChild = UIComponentFindChildByName(component, "quantity text");
	UI_LABELEX *pQuantityText = (pChild ? UICastToLabel(pChild) : NULL);
	if (pQuantityText)
	{
		UIComponentRemoveAllElements(pQuantityText);
	}
	if (pLitFrameComponent)
	{
		UIComponentRemoveAllElements(pLitFrameComponent);
	}

	UIComponentAddFrame(component);

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	ASSERT_RETVAL(texture, UIMSG_RET_NOT_HANDLED);
	BOOL Highlighted = FALSE;

	// show or hide the background icon
	if( AppIsHellgate() )
	{
		if (pSlotIcon)
		{
			BOOL bShow = item == NULL || UnitGetId(item) == UIGetCursorUnit();
			UIComponentSetVisible(pSlotIcon, bShow);
		}
	}
	else
	{
		UI_COMPONENT* pBkgComponent = UIComponentFindChildByName(component, "background");
		BOOL bShow = item == NULL || UnitGetId(item) == UIGetCursorUnit();
		UIComponentSetVisible(pBkgComponent, bShow);
	}

	DWORD dwOkColor = (AppIsTugboat() ? GFXCOLOR_WHITE : GetFontColor(FONTCOLOR_HILIGHT_BKGD) );
	BOOL bNoStretch = FALSE; //!AppIsTugboat();
	float fScaleX = (AppIsTugboat() ? invslot->m_fWidth / invslot->m_pLitFrame->m_fWidth : 1.0f);
	float fScaleY = (AppIsTugboat() ? invslot->m_fHeight / invslot->m_pLitFrame->m_fHeight : 1.0f);

	if (!item)
	{
		if (invslot->m_nInvLocation != INVALID_LINK)
		{
			if (UIGetCursorUnit() == INVALID_ID)
			{
				return UIMSG_RET_HANDLED;
			}

			UNIT* pCursorItem = UnitGetById(AppGetCltGame(), UIGetCursorUnit());
			if (!pCursorItem)
			{
				return UIMSG_RET_HANDLED;
			}
			if (!UnitInventoryTestIgnoreExisting(container, pCursorItem, invslot->m_nInvLocation))
			{
				return UIMSG_RET_HANDLED;
			}

			DWORD dwItemEquipFlags = 0;
			SETBIT( dwItemEquipFlags, IEF_IGNORE_PLAYER_PUT_RESTRICTED_BIT );
			BOOL bMeetsRequirements = InventoryItemCanBeEquipped(container, pCursorItem, invslot->m_nInvLocation, NULL, 0, dwItemEquipFlags);
			if (invslot->m_pLitFrame)
			{
				UIComponentAddElement(pLitFrameComponent, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), bMeetsRequirements ? dwOkColor : GFXCOLOR_RED, NULL, bNoStretch, fScaleX, fScaleY);
				Highlighted = TRUE;
			}
			else
			{
				UIComponentAddBoxElement(pLitFrameComponent, 0.0f, 0.0f, invslot->m_fHeight, invslot->m_fHeight, NULL, bMeetsRequirements ? UI_HILITE_GREEN : GFXCOLOR_RED, 24);
			}
		}

		return UIMSG_RET_HANDLED;
	}


	UNIT* pCursorUnit = NULL;
	if (UIGetCursorUnit() != INVALID_ID &&
		invslot->m_nInvLocation != INVALID_LINK)
	{
		pCursorUnit = UnitGetById(AppGetCltGame(), UIGetCursorUnit());
		if (pCursorUnit)
		{
			UNIT* pItemOwner = UnitGetUltimateOwner(pCursorUnit);
			// if the cursor unit is owned by another unit (like a merchant) and
			// if there's already an item in this slot don't do anything else
			if (pItemOwner != container && 
				pItemOwner != UnitGetUltimateOwner(container) &&
				sUIInvSlotGetItem(container, invslot) != NULL)
			{
			}
			else if (UnitInventoryTestIgnoreExisting(container, pCursorUnit, invslot->m_nInvLocation) &&
					 !UnitGetStat(item, STATS_ITEM_LOCKED_IN_INVENTORY))
			{
				BOOL bMeetsRequirements = InventoryCheckStatRequirements(container, pCursorUnit, item, NULL, invslot->m_nInvLocation);
				if (invslot->m_pLitFrame)
				{
					UIComponentAddElement(pLitFrameComponent, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), bMeetsRequirements ? dwOkColor : GFXCOLOR_RED, NULL, bNoStretch, fScaleX, fScaleY);
					Highlighted = TRUE;
				}
				else
				{
					UIComponentAddBoxElement(pLitFrameComponent, 0.0f, 0.0f, invslot->m_fWidth, invslot->m_fHeight, NULL, bMeetsRequirements ? UI_HILITE_GREEN : GFXCOLOR_RED, 24);
				}
			}
			else if (UnitIsA(pCursorUnit, UNITTYPE_MOD) && AppIsHellgate() )
			{
				//Highlight this item if the mod can go in it
				if( pOverlayComponent)
				{
					int nLocation = INVLOC_NONE;
					BOOL bOpenSpace = ItemGetOpenLocation(item, pCursorUnit, FALSE, &nLocation, NULL, NULL);
					if (nLocation != INVLOC_NONE)
					{
						BOOL bMeetsRequirements = InventoryCheckStatRequirements(item, pCursorUnit, NULL, NULL, nLocation);
						if (!bOpenSpace || !bMeetsRequirements)
						{
							UIComponentAddPrimitiveElement(pOverlayComponent, UIPRIM_BOX, 0, UI_RECT(0.0f, 0.0f, pOverlayComponent->m_fWidth, pOverlayComponent->m_fHeight), NULL, NULL, invslot->m_dwModWontFitColor);
						}
					}
					else
					{
						UIComponentAddPrimitiveElement(pOverlayComponent, UIPRIM_BOX, 0, UI_RECT(0.0f, 0.0f, pOverlayComponent->m_fWidth, pOverlayComponent->m_fHeight), NULL, NULL, invslot->m_dwModWontGoInItemColor);
					}
				}
				
			}
		}
	}
	// if this is the slot we've got the mouse over, highlight it
	else if (UnitGetId(item) == UIGetHoverUnit())
	{
		if (invslot->m_pLitFrame)
		{
			if( AppIsTugboat() )
			{
				UIComponentAddElement(component, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_WHITE, NULL, bNoStretch, fScaleX, fScaleY);
				Highlighted = TRUE;
			}
			else 
			{
				UIComponentAddElement(component, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), GetFontColor(FONTCOLOR_HILIGHT_BKGD), NULL, bNoStretch, fScaleX, fScaleY);
			}
		}
	}

	if (item && UnitIsWaitingForInventoryMove(item))
	{
		return UIMSG_RET_HANDLED;
	}

	if (invslot->m_pLitFrame)
	{
		// draw background for slot
		if (AppIsHellgate())
		{
			int color = UIInvGetItemGridBackgroundColor(item);
			if (color != INVALID_ID)
			{
				UIComponentAddElement(pLitFrameComponent, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), GetFontColor(color), NULL, bNoStretch);
			}
		}
		else if( !Highlighted )
		{
			DWORD dwColor = GFXCOLOR_WHITE;
			int color = UIInvGetItemGridBackgroundColor(item);
			if (color > 0)
			{
				dwColor = GetFontColor(color);
			}

			if( dwColor != GFXCOLOR_WHITE )
			{
				UIComponentAddElement(component, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), dwColor, NULL, bNoStretch, fScaleX, fScaleY);				
				Highlighted = TRUE;
			}
			
		}
	}

	UI_TEXTURE_FRAME *pIconFrame = NULL;
	int nColorIcon;
	int nBorderColor = UIInvGetItemGridBorderColor(item, invslot, &pIconFrame, &nColorIcon);
	if (pIconFrame)
	{
		UI_COMPONENT *pIconPanel = UIComponentFindChildByName(invslot, "icon panel");
		UIX_TEXTURE *pIconTexture = (pIconPanel ? UIComponentGetTexture(pIconPanel) : NULL);
		if (pIconPanel && pIconTexture)
		{
			UI_POSITION posIcon(invslot->m_fWidth - (pIconFrame->m_fWidth + 5.0f), invslot->m_fHeight - (pIconFrame->m_fHeight + 5.0f));
			UI_RECT rectComponent = UIComponentGetRect(pIconPanel);
			UIComponentAddElement(pIconPanel, pIconTexture, pIconFrame, posIcon, GetFontColor(nColorIcon), NULL, TRUE);
		}
	}

	if (nBorderColor != INVALID_ID && invslot->m_pUseableBorderFrame)
	{
		UI_RECT rectComponent = UIComponentGetRectBase(invslot);
		UIComponentAddElement(component, texture, invslot->m_pUseableBorderFrame, UI_POSITION(0.0f, 0.0f), GetFontColor(nBorderColor), &rectComponent, FALSE, fScaleX, fScaleY);
	}

	if (!AppIsTugboat())
    {
		// KCK: In some cases (such as remote player inspection), we've not yet created the
		// viewable inventory item. If we haven't, do so now:
		if (c_UnitGetModelIdInventory(item) == INVALID_ID)
			UIItemInitInventoryGfx(item, FALSE, FALSE);

	    UIComponentAddItemGFXElement(component, item, 
		    UI_RECT(invslot->m_fModelBorderX, invslot->m_fModelBorderY, invslot->m_fWidth - invslot->m_fModelBorderX, invslot->m_fHeight - invslot->m_fModelBorderY), NULL, &invslot->m_ItemAdj);
    }
    else
    {
	    UI_GFXELEMENT* element = NULL;
	    if( item->pIconTexture )
	    {
			BOOL bLoaded = UITextureLoadFinalize( item->pIconTexture );
			REF( bLoaded );

		    UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(item->pIconTexture->m_pFrames, "icon");
		    if (frame && bLoaded )
			{
				float OffsetX =  invslot->m_fWidth / 2 - item->pUnitData->nInvWidth * ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale / 2;
				float OffsetY =  invslot->m_fHeight / 2 - item->pUnitData->nInvHeight * ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale / 2;
				UI_SIZE sizeIcon;
				sizeIcon.m_fWidth = item->pUnitData->nInvWidth * ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale;
				sizeIcon.m_fHeight = item->pUnitData->nInvHeight * ICON_BASE_TEXTURE_WIDTH * invslot->m_fSlotScale;

				if( AppIsHellgate() )
				{
					element = UIComponentAddElement(component, item->pIconTexture, frame, UI_POSITION(OffsetX, OffsetY) , GFXCOLOR_WHITE, &UI_RECT(0.0f, 0.0f, invslot->m_fWidth, invslot->m_fHeight), 0, 1.0f, 1.0f, &sizeIcon  );//, GetFontColor(pSkillData->nIconColor), NULL, FALSE, sSmallSkillIconScale());
				}
				else
				{
					BOOL bIsEquipped = ItemIsEquipped( item, container );
					element = UIComponentAddElement(component, item->pIconTexture, frame, UI_POSITION(OffsetX, OffsetY) , bIsEquipped ? GFXCOLOR_WHITE : GFXCOLOR_DKGRAY, &UI_RECT(0.0f, 0.0f, invslot->m_fWidth, invslot->m_fHeight), 0, 1.0f, 1.0f, &sizeIcon  );//, GetFontColor(pSkillData->nIconColor), NULL, FALSE, sSmallSkillIconScale());
					
				}
				if (UnitGetId(item) == UIGetCursorUnit())
				{
					element->m_dwColor = UI_ITEM_GRAY;
				}
			}
			else 
			{
				// it's not asynch loaded yet - schedule a repaint so we can take care of it
				if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
				{
					CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
				}
				component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_GAME_TICK / 10, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR)component, UIMSG_PAINT, wParam, lParam));
			}
	    }
		//draw the gems and mod slots for tugboat
		sUIX_DrawGemsAndMods( NULL,
							  invslot,
							  item );

    }

	if (pQuantityText)
	{
		int nQuantity = ItemGetQuantity( item );
		if (nQuantity > 1)
		{
			const int MAX_STRING = 32;
			WCHAR szBuff[ MAX_STRING ];
			LanguageFormatIntString( szBuff, MAX_STRING, nQuantity );
			UI_RECT rectComponent = UIComponentGetRect(pQuantityText);
			UIComponentAddTextElement(pQuantityText, UIComponentGetTexture(pQuantityText), 
				UIComponentGetFont(pQuantityText), UIComponentGetFontSize(pQuantityText), 
				szBuff, UI_POSITION(0, 0), pQuantityText->m_dwColor, &rectComponent, pQuantityText->m_nAlignment);
		}
	}

	if (pPriceText)
	{
		cCurrency sellPrice = (bIsMerchant ? EconomyItemBuyPrice(item) : EconomyItemSellPrice(item));
		
		if (sellPrice.IsZero() != FALSE )
		{
			int nPrice = sellPrice.GetValue( KCURRENCY_VALUE_INGAME );
			const int MAX_MONEY_STRING = 32;
			WCHAR szBuff[ MAX_MONEY_STRING ];
			UIMoneyGetString( szBuff, MAX_MONEY_STRING, nPrice );
			
			DWORD dwColor = pPriceText->m_dwColor;
			if (bIsMerchant && UnitGetCurrency(UIGetControlUnit()) < sellPrice)
			{
				dwColor = GFXCOLOR_RED;
			}
			UIComponentAddTextElement(pPriceText, UIComponentGetTexture(pPriceText), 
				UIComponentGetFont(pPriceText), UIComponentGetFontSize(pPriceText), 
				szBuff, UI_POSITION(), dwColor, &UIComponentGetRect(invslot), pPriceText->m_nAlignment);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnMouseMove(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVSLOT* invslot = UICastToInvSlot(component);

	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

	UNIT* item = NULL;
	UNIT* container = NULL;
	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		container = UIGetControlUnit();
		item = UIComponentGetFocusUnit(invslot);
	}
	else
	{
		container = UIComponentGetFocusUnit(invslot);
		item = sUIInvSlotGetItem(container, invslot);
	}

	if (!item || invslot->m_nInvLocation == INVALID_LINK)
	{
		UISetHoverUnit(INVALID_ID, UIComponentGetId(invslot));
	}
	else
	{
		UISetHoverUnit(UnitGetId(item), UIComponentGetId(invslot));
	}
	if( AppIsTugboat() )
	{
		UIInvSlotOnInventoryChange(invslot, UIMSG_INVENTORYCHANGE, UIComponentGetFocusUnitId(invslot), invslot->m_nInvLocation);
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_INVSLOT* invslot = UICastToInvSlot(component);

	if (!UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* item = NULL;
	UNIT* container = NULL;
	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		container = UIGetControlUnit();
		item = UIComponentGetFocusUnit(invslot);
	}
	else
	{
		container = UIComponentGetFocusUnit(invslot);
		item = sUIInvSlotGetItem(container, invslot);
	}

	if (!item)
	{
		UISetHoverUnit(INVALID_ID, UIComponentGetId(invslot));
		if (!AppIsTugboat() &&
			msg == UIMSG_MOUSEHOVERLONG)
		{
			UIComponentMouseHoverLong(component, msg, wParam, lParam);
		}
	}
	else
	{
		UISetHoverTextItem(component, item, invslot->m_ItemAdj.fScale);
		if (invslot->m_nInvLocation != INVALID_LINK)
			UISetHoverUnit(UnitGetId(item), UIComponentGetId(invslot));
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component)
	{
		UIClearHoverText();
		UISetHoverUnit(INVALID_ID, UIComponentGetId(component));
	}
	if( AppIsTugboat() )
	{
		UI_INVSLOT* invslot = UICastToInvSlot(component);

		UIInvSlotOnInventoryChange(invslot, UIMSG_INVENTORYCHANGE, UIComponentGetFocusUnitId(invslot), invslot->m_nInvLocation);
	}
	else
	{
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIInvSlotOnLButtonDownNoCursorUnit(
	UI_INVSLOT* invslot)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	if (!UIComponentCheckBounds(invslot, x, y, &pos) || !UIComponentGetVisible(invslot))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* container = UIComponentGetFocusUnit(invslot);
	if (!container ||  
		(UnitIsPlayer(container) && container != UIGetControlUnit()) )	// KCK: Don't allow picking up other people's items
	{
		return UIMSG_RET_END_PROCESS;
	}

	int location = invslot->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		return UIMSG_RET_END_PROCESS;
	}

	UNIT* item = sUIInvSlotGetItem(container, invslot);

	if (AppIsTugboat())
	{
		// items can be shift-left-clicked to be sold/bought
		if ( UIIsMerchantScreenUp() &&
			(GetKeyState(VK_SHIFT) & 0x8000)	)
		{
			UIDoMerchantAction(item);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	if (item &&
		!UnitGetStat(item, STATS_ITEM_LOCKED_IN_INVENTORY))
	{
		
		if (AppIsHellgate())
	    {
			int nWeaponConfig = -1;
			if (GetWeaponconfigCorrespondingInvLoc(0) == invslot->m_nInvLocation ||
				GetWeaponconfigCorrespondingInvLoc(1) == invslot->m_nInvLocation)
			{
				nWeaponConfig = UnitGetStat(container, STATS_CURRENT_WEAPONCONFIG) + TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
			}
		    UISetCursorUnit(UnitGetId(item), 
				TRUE, 
				nWeaponConfig, 
			    (pos.m_fX - x) / invslot->m_fWidth, 
			    (pos.m_fY - y) / invslot->m_fHeight);
		    UIPlayItemPickupSound(item);
	    }
		else
        {
        
	        // where will the item go
	        int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_CURSOR );
			c_InventoryTryPut( UIGetControlUnit(), item, nInventorySlot, 0, 0 );
	        UnitWaitingForInventoryMove(item, TRUE);
	        UISetCursorUnit(UnitGetId(item), TRUE);
	        UIPlayItemPickupSound(item);
        }
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIInvSlotOnLButtonDownWithCursorUnit(
	UI_INVSLOT* invslot)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(invslot, x, y, &pos)|| !UIComponentGetVisible(invslot))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNITID idCursorUnit = UIGetCursorUnit();
	UNIT* unitCursor = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!unitCursor)
	{
		UIClearCursorUnitReferenceOnly();
		return UIMSG_RET_END_PROCESS;
	}

	UNIT* container = UIComponentGetFocusUnit(invslot);
	if (!container)
	{
		UIClearCursor();
		return UIMSG_RET_END_PROCESS;
	}

	int location = invslot->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		UIClearCursor();
		return UIMSG_RET_END_PROCESS;
	}

	if( AppIsTugboat() )
	{
		// hide any confirmations we've got up - we just dropped something!
		UIHideGenericDialog();
	}

	UNIT* item = sUIInvSlotGetItem(container, invslot);

	// check for store interactions
	if (ItemBelongsToAnyMerchant(unitCursor))
	{
		// moving item from merchant to player inventory
		BOOL bBought = UIDoMerchantAction(unitCursor, location, 0, 0);
		if (bBought)
		{
		    UISetCursorUnit(INVALID_ID, TRUE);
		}

		if (bBought && UnitIsA(unitCursor, UNITTYPE_MOD) && UnitGetInventoryType(item) > 0)
		{
			// fall through and place the mod
		}
		else
		{
			// no more processing to do
			return UIMSG_RET_HANDLED_END_PROCESS;						
		}
	}
	
	if (item && UnitGetStat(item, STATS_ITEM_LOCKED_IN_INVENTORY))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	// if cursor is a mod, and item clicked on can have an inventory...
	if (item && UnitIsA(unitCursor, UNITTYPE_MOD) && UnitGetInventoryType(item) > 0)
	{
		if( AppIsTugboat() )
		{
			sUISocketedItemOnLButtonDownWithCursorUnit( item );

			return UIMSG_RET_HANDLED_END_PROCESS;

		}
		UIPlaceMod(item, unitCursor, !UIIsGunModScreenUp());

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (!IsAllowedUnitInv(unitCursor, location, container))
	{
		return UIMSG_RET_HANDLED;	// Fall out and try to auto-equip on the paperdoll down handler
	}

	if (!UICheckRequirementsWithFailMessage(container, unitCursor, item, location, invslot))
	{
		return UIMSG_RET_HANDLED;	// Fall out and try to auto-equip on the paperdoll down handler
	}

	ASSERT(g_UI.m_Cursor);
	invslot->m_ItemAdj.fXOffset = ((x - (g_UI.m_Cursor->m_fWidth/2.0f)) - pos.m_fX);
	invslot->m_ItemAdj.fYOffset = ((y - (g_UI.m_Cursor->m_fHeight/2.0f)) - pos.m_fY);
	invslot->m_ItemAdj.fScale = 2.0f;		// TODO: change this

	invslot->m_ItemAdjStart = invslot->m_ItemAdj;

	int nFromWeaponConfig = g_UI.m_Cursor->m_nFromWeaponConfig;
	
	if (invslot->m_tItemAdjAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(invslot->m_tItemAdjAnimInfo.m_dwAnimTicket);
	}
	invslot->m_tItemAdjAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIInvslotDeflate, CEVENT_DATA((DWORD_PTR)invslot, 0));	

	if (!item)
	{
		if (UnitInventoryTest(container, unitCursor, location))
		{
			c_InventoryTryPut( container, unitCursor, location, 0, 0 );
			UIClearCursorUnitReferenceOnly();
			UIPlayItemPutdownSound(unitCursor, container, location);
			UnitWaitingForInventoryMove(unitCursor, TRUE);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		else
		{
			int prev_loc;
			if (IsUnitPreventedFromInvLoc(container, unitCursor, location, &prev_loc))
			{
				UNIT* prev_item = UnitInventoryGetByLocation(container, prev_loc);
				if (prev_item)
				{
					c_InventoryTrySwap(unitCursor, prev_item, nFromWeaponConfig);
				    UIPlayItemPutdownSound(unitCursor, container, location);
				}
			}
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	if (item == unitCursor)
	{
		UIPlayItemPutdownSound(unitCursor, container, location);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	// check if a swap is possible
	if (!UnitInventoryTestSwap(item, unitCursor))
	{
		int prev_loc;
		if (!IsUnitPreventedFromInvLoc(container, unitCursor, location, &prev_loc))
		{
			return UIMSG_RET_HANDLED;	// Fall out and try to auto-equip on the paperdoll down handler
		}
        UNIT* prev_item = UnitInventoryGetByLocation(container, prev_loc);
		if (!prev_item)
		{
			return UIMSG_RET_HANDLED;	// Fall out and try to auto-equip on the paperdoll down handler
		}
		int new_loc, new_x, new_y;
		if (!ItemGetOpenLocation(container, prev_item, TRUE, &new_loc, &new_x, &new_y))
		{
			return UIMSG_RET_HANDLED;	// Fall out and try to auto-equip on the paperdoll down handler
		}

		c_InventoryTryPut( container, prev_item, new_loc, new_x, new_y );
		UnitWaitingForInventoryMove(prev_item, TRUE);

		c_InventoryTrySwap(unitCursor, item, nFromWeaponConfig);
		UIPlayItemPutdownSound(unitCursor, container, new_loc);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	c_InventoryTrySwap(unitCursor, item, nFromWeaponConfig);

	if (!AppIsTugboat())
    {
	    g_UI.m_Cursor->m_tItemAdjust.fScale = 1.5f;
	    UIGetStdItemCursorSize(item, g_UI.m_Cursor->m_fItemDrawWidth, g_UI.m_Cursor->m_fItemDrawHeight);
    
	    g_UI.m_Cursor->m_tItemAdjust.fXOffset = -0.5f * g_UI.m_Cursor->m_fItemDrawWidth;
	    g_UI.m_Cursor->m_tItemAdjust.fYOffset = -0.5f * g_UI.m_Cursor->m_fItemDrawHeight;
    }

	UIPlayItemPutdownSound(unitCursor, container, location);
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	if (UIComponentCheckBounds(component, x, y, &pos) && UIComponentGetVisible(component))
	{
		if (UIGetCursorSkill() != INVALID_ID && UIComponentCheckBounds( component ))
			return UIMSG_RET_HANDLED_END_PROCESS;
	}



	UI_INVSLOT* invslot = UICastToInvSlot(component);
	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_MSG_RETVAL eResult;
	
	if (UIGetCursorUnit() == INVALID_ID)
	{
		eResult = sUIInvSlotOnLButtonDownNoCursorUnit(invslot);
	}
	else
	{
		eResult = sUIInvSlotOnLButtonDownWithCursorUnit(invslot);
	}
	
	return eResult;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnRButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_INVSLOT* invslot = UICastToInvSlot(component);

	if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (UIGetCursorUnit() != INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	float x, y;
	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	if (!UIComponentCheckBounds(invslot, x, y, &pos) || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* item = NULL;
	UNIT* container = NULL;
	if (invslot->m_nInvLocation == INVALID_LINK)
	{
		container = UIGetControlUnit();
		item = UIComponentGetFocusUnit(invslot);
	}
	else
	{
		container = UIComponentGetFocusUnit(invslot);
		item = sUIInvSlotGetItem(container, invslot);
	}

	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!item)
	{
		return UIMSG_RET_HANDLED;
	}

	if( AppIsTugboat() )
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	else if (UIIsMerchantScreenUp() &&	
		( (GetKeyState(VK_SHIFT) & 0x8000) &&		// the shift key is down
		!UnitInventoryIterate(item, NULL)))	// if the item does not contain any other items (mods)
	{
		// buy or sell the item
		if (UIDoMerchantAction(item))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	else if (UIIsStashScreenUp() &&
		(GetKeyState(VK_SHIFT) & 0x8000))		// the shift key is down
	{
		// try to put the item in your stash
		if (!sInvLocTryPut( container, item, GlobalIndexGet( GI_INVENTORY_LOCATION_STASH) ))
		{
			if (PlayerIsSubscriber(container))
			{
				sInvLocTryPut( container, item, GlobalIndexGet( GI_INVENTORY_LOCATION_EXTRA_STASH) );
			}
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	else if (AppIsHellgate())
	{
		c_InteractRequestChoices(item, component);

		//// We're not going to the server to get the possible choices right now because the client
		////   knows all it needs to about the items to display the radial menu.

		//INTERACT_MENU_CHOICE ptInteractChoices[MAX_INTERACT_CHOICES];
		//int nChoicesCount = 0;
		//ItemGetInteractChoices(UIGetControlUnit(), item, ptInteractChoices, nChoicesCount);
		//c_InteractDisplayChoices(UIGetControlUnit(), item, ptInteractChoices, nChoicesCount );

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInvLocIsVisible(
	UNIT *pContainer,
	int nInventorySlot)
{
	if (nInventorySlot == INVALID_LINK)
	{
		return FALSE;
	}
	
	const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pContainer, nInventorySlot );
	if (pInvLocData == NULL)
	{
		return FALSE;
	}

	if (pInvLocData->nAllowTypes[ 0 ] == INVALID_LINK || pInvLocData->nAllowTypes[ 0 ] == 0)
	{
	
		UNIT * pUnitToTest = pContainer;
		if ( TESTBIT( pInvLocData->dwFlags, INVLOCFLAG_SKILL_CHECK_ON_ULTIMATE_OWNER ) )
		{
			pUnitToTest = UnitGetUltimateOwner( pContainer );
		}
		if ( ! pUnitToTest )
		{
			return FALSE;
		}

		// see if the unit has a skill which will give them an exemption
		for (int i=0; i < INVLOC_NUM_ALLOWTYPES_SKILLS; i++)
		{
			// it's the right type.  Now see if they have the skill
			if (pInvLocData->nAllowTypeSkill[i] != INVALID_LINK)
			{
				GAME * pGame = UnitGetGame( pUnitToTest );
				const SKILL_DATA * pSkillData = SkillGetData( pGame, pInvLocData->nAllowTypeSkill[ i ] );
				if ( ! pSkillData || 
					SkillGetLevel(pUnitToTest, pInvLocData->nAllowTypeSkill[i]) < pInvLocData->nSkillAllowLevels[ i ] ||
					! UnitIsA( pUnitToTest, pSkillData->nRequiredUnittype ) )
				{
					return FALSE;
				}
			}
		}

	}
		
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInvSlotOnSetFocusUnit(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERTX_RETVAL( pComponent, UIMSG_RET_NOT_HANDLED, "Expected component" );
		
	// get focus unit
	BOOL bShowSlot = TRUE;
	UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
	if (pUnit)
	{
		UI_INVSLOT *pInvSlot = UICastToInvSlot( pComponent );
		if (pInvSlot->m_nInvLocation != INVALID_LINK)
		{
		
			// check to see if we have this inventory location
			const INVLOC_DATA *pInvLocData = UnitGetInvLocData( pUnit, pInvSlot->m_nInvLocation );
			if (pInvLocData == NULL)
			{
				bShowSlot = FALSE;
			}

			// check for skill requirements for this inventory slot
			if (sInvLocIsVisible( pUnit, pInvSlot->m_nInvLocation ) == FALSE)
			{
				bShowSlot = FALSE;
			}

		}
						
	}
	
	if (bShowSlot)
	{
		UIComponentSetActive( pComponent, TRUE );		
	}
	else
	{
		UIComponentSetVisible( pComponent, FALSE );
	}

	return UIMSG_RET_HANDLED;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnPostActivateSkillMap(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);

	if (msg == UIMSG_POSTACTIVATE)
	{
		if (component && UIComponentGetActive(component))
		{
			c_QuestEventOpenUI( UIE_SKILL_MAP );	
		}

/*		UI_COMPONENT *pCloseButton = UIComponentGetById(UIComponentGetIdByName("skills close button"));
		if (pCloseButton)
		{
			UNIT *pPlayer = UIComponentGetFocusUnit(component);
			UIComponentSetActive(pCloseButton, c_TutorialInputOk(pPlayer, TUTORIAL_INPUT_SKILLS_CLOSE));
			UIComponentHandleUIMessage(pCloseButton, UIMSG_PAINT, 0, 0);
		}*/
	}

	UIStopAllSkillRequests();
	c_PlayerClearAllMovement(AppGetCltGame());

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnPostInactivateSkillMap(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate() && component)
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(component);
		if (pPlayer) c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_SKILLS_CLOSE );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sGetStateDurationIndex(
	int nState,
	BOOL bAdd)
{
	int nFirstEmpty = -1;
	for (int i=0; i<MAX_STATES_DISPLAY_ITEMS; i++)
	{
		if (pgnTrackingStates[i] == nState)
		{
			return i;
		}
		if (pgnTrackingStates[i] == -1 && 
			nFirstEmpty == -1)
		{
			nFirstEmpty = i;
		}
	}

	if (bAdd && nFirstEmpty > -1 && nFirstEmpty < MAX_STATES_DISPLAY_ITEMS)
	{
		pgnTrackingStates[nFirstEmpty] = nState;
		return nFirstEmpty;
	}

	return -1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STATE_ICON
{
	const STATE_DATA *	m_Data;
	unsigned int		m_nState;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sStateIconCompare(
	const void * item1,
	const void * item2)
{
	const STATE_ICON * a = (const STATE_ICON *)item1;
	const STATE_ICON * b = (const STATE_ICON *)item2;

	// return 1 if a is bad and b isn't or -1 if b is bad and a isn't
	if (StateDataTestFlag( a->m_Data, SDF_IS_BAD ) != StateDataTestFlag( b->m_Data, SDF_IS_BAD ))
	{
		return (StateDataTestFlag( a->m_Data, SDF_IS_BAD ) ? 1 : -1);
	}
	if (a->m_Data->nIconOrder < b->m_Data->nIconOrder)
	{
		return -1;
	}
	else if (a->m_Data->nIconOrder > b->m_Data->nIconOrder)
	{
		return 1;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIStatesDisplayOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT(component);

	float fIconScale = 1.0f;

	if (component->m_dwParam2 > 0)
	{
		fIconScale = (float)component->m_dwParam2 / 100.0f;
	}

	static const float fSPACING = 5.0f;
	UI_SIZE sizeIcon(64.0f * fIconScale * ( AppIsHellgate() ? UIGetAspectRatio() : 1.0f ), 64.0f * fIconScale);

	UIComponentRemoveAllElements(component);	
	UIComponentAddFrame(component);	

	GAME* pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	UNIT *pUnit = UIComponentGetFocusUnit(component);
	if (!pUnit)
		return UIMSG_RET_NOT_HANDLED;

	DWORD dwCurrentTick = GameGetTick(pGame);
	if (msg == UIMSG_SETCONTROLSTATE)
	{
		ASSERT_RETVAL(lParam, UIMSG_RET_NOT_HANDLED);
		UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)CAST_TO_VOIDPTR(lParam);

		if (pData->m_bClearing) 
		{
			int nIdx = sGetStateDurationIndex(pData->m_nState, FALSE);
			if (nIdx > -1)
			{
				pgdwGameTickStateEnds[nIdx] = 0;
				pgnTrackingStates[nIdx] = -1;
			}
		}
		else if (pData->m_nDuration > 0)
		{
			int nIdx = sGetStateDurationIndex(pData->m_nState, TRUE);
			if (nIdx > -1)
			{
				pgdwGameTickStateEnds[nIdx] = dwCurrentTick + pData->m_nDuration;
			}
		}
	}

	UIX_TEXTURE *pTexture = UIComponentGetTexture(UIComponentGetByEnum(UICOMP_SKILLGRID1));
	if (!pTexture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	BOOL bScheduleAnother = FALSE;
	if (msg == UIMSG_RESTART)
	{
		bScheduleAnother = TRUE;
	}

	UI_RECT rect = 	UI_RECT ( 0.0f, 
							  0.0f,
							  component->m_fWidth,
							  component->m_fHeight );

	UI_POSITION pos;

	static const int MAX_STATE_ICONS = 64;
	STATE_ICON stateicons[MAX_STATE_ICONS];
	unsigned int statecount = 0;

	{	// first: get all states w/ icons
		UNIT_ITERATE_STATS_RANGE(pUnit, STATS_STATE, statsentry, ii, 64)
		{
			ASSERT_BREAK(statecount < MAX_STATE_ICONS);

			const STATE_DATA * state_data = (const STATE_DATA *)ExcelGetData(pGame, DATATABLE_STATES, statsentry[ii].GetParam());
			if (!state_data || !state_data->szUIIconFrame || !state_data->szUIIconFrame[0])
			{
				continue;
			}
			stateicons[statecount].m_Data = state_data;
			stateicons[statecount].m_nState = statsentry[ii].GetParam();
			statecount++;
		}
		UNIT_ITERATE_STATS_END;
	}

	{	// now sort all the states in draw order
		qsort(stateicons, statecount, sizeof(STATE_ICON), sStateIconCompare);
	}

	for (int bBad = 0; bBad <= 1; ++bBad)
	{
		if (component->m_dwParam == 1)		// right-justify
		{
			pos.m_fX = rect.m_fX2;
		}
		else
		{
			pos.m_fX = 0.0f;
		}
		float fMaxHeight = 0.0f;

		// now iterate all states == bBad and draw them
		for (unsigned int ii = 0; ii < statecount; ++ii)
		{
			const STATE_DATA * pStateData = stateicons[ii].m_Data;
			
			if (!pStateData)
				continue;

			if (StateDataTestFlag( pStateData, SDF_IS_BAD ) != bBad)
				continue;

			if (!pStateData->szUIIconFrame || !pStateData->szUIIconFrame[0])
				continue;

			int state = stateicons[ii].m_nState;

			UIX_TEXTURE* pStateTexture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, pStateData->szUIIconTexture);
			if (!pStateTexture)
			{
				continue;
			}
			UI_TEXTURE_FRAME* pStateIcon = (UI_TEXTURE_FRAME*)StrDictionaryFind(pStateTexture->m_pFrames, pStateData->szUIIconFrame);
			if (!pStateIcon)
			{
				continue;
			}

			if (component->m_dwParam == 1)		// right-justify
			{
				pos.m_fX -= ( (sizeIcon.m_fWidth ) + fSPACING );
				if (pos.m_fX < 0.0f)		// wrap to the next line
				{
					pos.m_fY += ( fMaxHeight + fSPACING );
					pos.m_fX = rect.m_fX2 - ((sizeIcon.m_fWidth ) + fSPACING);
				}
			}

			UI_TEXTURE_FRAME* pIconBack = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, pStateData->szUIIconBackFrame);
			if (pIconBack)
			{
				DWORD dwColor = GetFontColor(pStateData->nIconBackColor);
				dwColor = UI_MAKE_COLOR(UIComponentGetAlpha(component), dwColor);
				UI_GFXELEMENT* pElement = UIComponentAddElement(component, pTexture, pIconBack, pos, dwColor, &rect, FALSE, 1.0f, 1.0f, &sizeIcon );
				ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);
				pElement->m_fXOffset = 0.0f;	// heheheheh tricky
				pElement->m_fYOffset = 0.0f;
			}

			DWORD dwColor = GetFontColor(pStateData->nIconColor);
			dwColor = UI_MAKE_COLOR(UIComponentGetAlpha(component), dwColor);
			UI_GFXELEMENT* pElement = UIComponentAddElement(component, pStateTexture, pStateIcon, pos, dwColor, &rect, FALSE, 1.0f, 1.0f, &sizeIcon );
			ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);
			pElement->m_fXOffset = 0.0f;	// heheheheh tricky
			pElement->m_fYOffset = 0.0f;
			
			int nString = StringGetForApp( pStateData->nIconTooltipStr );
			pElement->m_qwData = (QWORD)nString;

			fMaxHeight = MAX(fMaxHeight, sizeIcon.m_fHeight );

			int nIdx = sGetStateDurationIndex(state, FALSE);
			if (nIdx > -1)
			{
				if (pgdwGameTickStateEnds[nIdx] > dwCurrentTick)
				{
					// this state has a duration.  Draw the time remaining.
					WCHAR szBuf[32] = { L"0" };
					int nTicksRemaining = pgdwGameTickStateEnds[nIdx] - dwCurrentTick;
					int nDurationSeconds = (nTicksRemaining + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND;
					LanguageGetDurationString(szBuf, arrsize(szBuf), nDurationSeconds);
					if (szBuf[0])
					{
						dwColor = UI_MAKE_COLOR(UIComponentGetAlpha(component), GFXCOLOR_WHITE);
						UIComponentAddTextElement(component, NULL, UIComponentGetFont(component), UIComponentGetFontSize(component), szBuf, pos, dwColor, NULL, UIALIGN_BOTTOMRIGHT, &sizeIcon);
					}

				}
				else
				{
					pgdwGameTickStateEnds[nIdx] = 0;
					pgnTrackingStates[nIdx] = -1;
				}

				// call again in a little bit to update the time remaining (or clear the state)
				bScheduleAnother = TRUE;
			}

			//else
			//{
			//	// Add some tiny text to ID the skill
			//	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			//	if (!pFont)
			//	{
			//		pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
			//	}
			//	UI_POSITION pos(rect.m_fX1, rect.m_fY1);
			//	UI_SIZE size(rect.m_fX2 - rect.m_fX1, rect.m_fY2 - rect.m_fY1);

			//	DWORD dwColor = UI_MAKE_COLOR(byAlpha, GFXCOLOR_WHITE);

			//	WCHAR szName[256];
			//	PStrCopy(szName, StringTableGetStringByIndex(pSkillData->nDisplayName), 256);
			//	DoWordWrap(szName, size.m_fWidth, pFont);
			//	UIComponentAddTextElement(component, pTexture, pFont, 0.0f, szName, pos, dwColor, &rect, UIALIGN_CENTER, &size );
			//}

			//if (pBorderFrame)
			//{
			//	DWORD dwColor = UI_MAKE_COLOR(MIN(byAlpha, (BYTE)(!bGreyout || bMouseOver ? 255 : 128)), GFXCOLOR_STD_UI_OUTLINE);
			//	UIComponentAddElement(component, pTexture, pBorderFrame, pos, dwColor, &rect);
			//}

			if (component->m_dwParam != 1)		// left-justify
			{
				pos.m_fX += sizeIcon.m_fWidth + fSPACING;
				if (pos.m_fX + sizeIcon.m_fWidth > rect.m_fX2)		// wrap to the next line
				{
					pos.m_fY += fMaxHeight + fSPACING;
					pos.m_fX = 0.0f;
				}
			}
		}
		pos.m_fY += fMaxHeight + fSPACING;
	}

	if (bScheduleAnother)
	{
		if (component->m_dwData)	// using dwData here because the animticket is used for sliding in and out
		{
			CSchedulerCancelEvent((DWORD&)component->m_dwData);
		}
		component->m_dwData = CSchedulerRegisterMessage(AppCommonGetCurTime() + 500, component, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatesDisplayOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if( AppIsTugboat() )
	{
		UIGetCursorPosition(&x, &y);
	}

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_POSITION pos;
	if (UIComponentCheckBounds(component, x, y, &pos))
	{
		x -= pos.m_fX;
		y -= pos.m_fY;
		x /= UIGetScreenToLogRatioX(component->m_bNoScale);
		y /= UIGetScreenToLogRatioY(component->m_bNoScale);

		UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
		while (pElement)
		{
			if ((int)pElement->m_qwData != INVALID_LINK &&
				UIElementCheckBounds(pElement, x, y))
			{
				const WCHAR * szTooltipText = StringTableGetStringByIndex((int)pElement->m_qwData);
				if (szTooltipText)
				{
					UISetSimpleHoverText(
						pos.m_fX + pElement->m_Position.m_fX,
						pos.m_fY + pElement->m_Position.m_fY,
						pos.m_fX + pElement->m_Position.m_fX + pElement->m_fWidth,
						pos.m_fY + pElement->m_Position.m_fY + pElement->m_fHeight,
						szTooltipText);
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
			pElement = pElement->m_pNextElement;
		}

		if( AppIsHellgate() )
		{
			UIClearHoverText();
		}
		return AppIsHellgate() ? UIMSG_RET_HANDLED_END_PROCESS : UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatesDisplayOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIClearHoverText();

	return AppIsHellgate() ? UIMSG_RET_HANDLED_END_PROCESS : UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotLabelOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component && component->m_pParent && UIComponentIsActiveSlot(component->m_pParent), UIMSG_RET_NOT_HANDLED);

	UI_ACTIVESLOT *pActiveSlot = UICastToActiveSlot(component->m_pParent);

	const KEY_COMMAND & tKey = InputGetKeyCommand(pActiveSlot->m_nKeyCommand);
	
	WCHAR szBuf[256];
	if (InputGetKeyNameW(tKey.KeyAssign[0].nKey, tKey.KeyAssign[0].nModifierKey, szBuf, arrsize(szBuf)))
	{
		if (PStrLen(szBuf) > 3)
		{
			szBuf[3] = L'*';
			szBuf[4] = L'\0';
		}

		UI_LABELEX* label = UICastToLabel(component);
		ASSERT_RETVAL(label, UIMSG_RET_NOT_HANDLED);
		UILabelSetText(label, szBuf, TRUE);

		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillIconLabelOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	const KEY_COMMAND & tKey = InputGetKeyCommand(CMD_SKILL_SHIFT);

	WCHAR szBuf[256];
	if (InputGetKeyNameW(tKey.KeyAssign[0].nKey, tKey.KeyAssign[0].nModifierKey, szBuf, arrsize(szBuf)))
	{
		if (PStrLen(szBuf) > 9)
		{
			szBuf[9] = L'*';
			szBuf[10] = L'\0';
		}

		UILabelSetText(component, szBuf);
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemIconLabelOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	const KEY_COMMAND & tKey = InputGetKeyCommand(CMD_SKILL_POTION);
	
	WCHAR szBuf[256];
	if (InputGetKeyNameW(tKey.KeyAssign[0].nKey, tKey.KeyAssign[0].nModifierKey, szBuf, arrsize(szBuf)))
	{
		if (PStrLen(szBuf) > 9)
		{
			szBuf[9] = L'*';
			szBuf[10] = L'\0';
		}

		UILabelSetText(component, szBuf);
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_ACTIVESLOT *pActiveSlot = UICastToActiveSlot(component);
	UI_COMPONENT *pIconPanel = pActiveSlot->m_pIconPanel;
	if (!pIconPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (msg == UIMSG_SETCONTROLSTAT && 
		STAT_GET_STAT(lParam) != STATS_POWER_CUR)
	{
		// we want to redraw when your power changes, but not the other stats.
		return UIMSG_RET_NOT_HANDLED;
	}

	WCHAR szQuantity[32] = L"";
	UI_COMPONENT *pQuantity = UIComponentFindChildByName(component, "ab quantity label");
	UI_COMPONENT *pHighlight = UIComponentFindChildByName(component, "icon highlight panel");
	UI_COMPONENT *pCooldown = UIComponentFindChildByName(component, "ab cooldown");
	UI_COMPONENT *pCooldownHighlight = UIComponentFindChildByName(component, "ab cooldown highlight");

	UIComponentRemoveAllElements(pActiveSlot);	
	UIComponentRemoveAllElements(pIconPanel);	// clear the slot's icon
	if (pCooldownHighlight )
		UIComponentRemoveAllElements(pCooldownHighlight);

	UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(pActiveSlot->m_nSlotNumber);
	BOOL bEmptyTag = (tag && tag->m_idItem[0] == INVALID_ID && 
		tag->m_idItem[1] == INVALID_ID &&
		tag->m_nSkillID[0] == INVALID_ID &&
		tag->m_nSkillID[1] == INVALID_ID);

	GAME *pGame = AppGetCltGame();
	UNIT *pFocusUnit = UIGetControlUnit();
	int nTicksRemaining = 0;
	BOOL bShowHighlight = FALSE;

	if (tag && 
		!bEmptyTag)
	{
		UNIT* item = (tag->m_idItem[0] != INVALID_ID ? UnitGetById(pGame, tag->m_idItem[0]) : NULL);
		UNIT* item2 = (tag->m_idItem[1] != INVALID_ID ? UnitGetById(pGame, tag->m_idItem[1]) : NULL);

		BOOL bGrayOut = FALSE;
		int nSkillID = tag->m_nSkillID[0];
		
		if ( item && item->pIconTexture )
		{
			BOOL bLoaded = UITextureLoadFinalize( item->pIconTexture );
			REF( bLoaded );
		}
		if ( item2 && item2->pIconTexture )
		{
			BOOL bLoaded = UITextureLoadFinalize( item2->pIconTexture );
			REF( bLoaded );
		}
		int nItemModelID1 = (item ? c_UnitGetModelIdInventory( item ) : INVALID_ID);
		int nItemModelID2 = (item2 ? c_UnitGetModelIdInventory( item2 ) : INVALID_ID);
		if( ( item && item->pIconTexture ) || ( item2 && item2->pIconTexture ) )
		{
			UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(item->pIconTexture->m_pFrames, "icon");
			if (frame)
			{
				float OffsetX =  0;
				float OffsetY =  0;
				UI_SIZE sizeIcon;
				sizeIcon.m_fWidth = pIconPanel->m_fWidth;
				sizeIcon.m_fHeight = pIconPanel->m_fHeight;

				if( item->pIconTexture )
				{
					UIComponentAddElement(pIconPanel, item->pIconTexture, frame, UI_POSITION(OffsetX, OffsetY) , GFXCOLOR_WHITE, &UI_RECT(0.0f, 0.0f, pIconPanel->m_fWidth, pIconPanel->m_fHeight), 0, 1.0f, 1.0f, &sizeIcon  );//, GetFontColor(pSkillData->nIconColor), NULL, FALSE, sSmallSkillIconScale());
				}
			}
		}
		else if (nItemModelID1 != INVALID_ID ||
				 nItemModelID2 != INVALID_ID)
		{
			if (nItemModelID1 != INVALID_ID && nItemModelID2 != INVALID_ID)
			{
				UIComponentAddModelElement(pIconPanel, UnitGetAppearanceDefIdForCamera( item ), nItemModelID1, 
					UI_RECT(pIconPanel->m_fWidth/2, 0.0f, pIconPanel->m_fWidth, pIconPanel->m_fHeight), 0.0f);
				UIComponentAddModelElement(pIconPanel, UnitGetAppearanceDefIdForCamera( item2 ), nItemModelID2, 
					UI_RECT(0.0f, 0.0f, pIconPanel->m_fWidth/2, pIconPanel->m_fHeight), 0.0f);
			}
			else if (nItemModelID1 != INVALID_ID)
			{
				UIComponentAddModelElement(pIconPanel, UnitGetAppearanceDefIdForCamera( item ), nItemModelID1, 
					UI_RECT(0.0f, 0.0f, pIconPanel->m_fWidth, pIconPanel->m_fHeight), 0.0f);
			}
			else if (nItemModelID2 != INVALID_ID)
			{
				UIComponentAddModelElement(pIconPanel, UnitGetAppearanceDefIdForCamera( item2 ), nItemModelID2, 
					UI_RECT(0.0f, 0.0f, pIconPanel->m_fWidth, pIconPanel->m_fHeight), 0.0f);
			}
		}
		else if (nSkillID != INVALID_ID)
		{
			//DWORD dwCurrentTick = GameGetTick(pGame);
			//if (pActiveSlot->m_dwTickCooldownOver > dwCurrentTick)
			//	nCooldownTicks = pActiveSlot->m_dwTickCooldownOver - dwCurrentTick;
			//else
			//	nCooldownTicks = 0;

			//BYTE bAlpha = (nCooldownTicks > 0 ? 128 : 255);
			BYTE bAlpha = 255;
			UI_POSITION pos(pIconPanel->m_fWidth/2.0f, pIconPanel->m_fHeight/2.0f);
			if( AppIsTugboat() )
			{
				pos = UI_POSITION( 0, 0 );
			}
			UI_RECT rect(pIconPanel->m_Position.m_fX, pIconPanel->m_Position.m_fY, pIconPanel->m_Position.m_fX + pIconPanel->m_fWidth, pIconPanel->m_Position.m_fY + pIconPanel->m_fHeight);

			//UI_TEXTURE_FRAME* pSkillIconBkgrd = UIGetStdSkillIconBackground(); 
			const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkillID );
			float fScale = 1.0f;
			BOOL bIconIsRed = FALSE;
			c_SkillGetIconInfo( pFocusUnit, nSkillID, bIconIsRed, fScale, nTicksRemaining, TRUE );
			UI_DRAWSKILLICON_PARAMS tUIDrawSkillIconParams(pIconPanel, rect, pos, nSkillID, /*pSkillIconBkgrd*/ NULL, NULL, NULL, NULL, bAlpha, FALSE, FALSE, FALSE, bIconIsRed, FALSE, 1.0f, 1.0f, pIconPanel->m_fWidth, pIconPanel->m_fHeight);
			tUIDrawSkillIconParams.bGreyout = SkillGetLevel( pFocusUnit, nSkillID ) == 0 && SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE );
			if ( tUIDrawSkillIconParams.bGreyout )
				tUIDrawSkillIconParams.bTooCostly = FALSE;
			UIDrawSkillIcon( tUIDrawSkillIconParams );

			if (pHighlight && pSkillData)
			{
				GAME * pGame = AppGetCltGame();
				// skill creates a state that is active (throb the highlight)
				if ( ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_ALWAYS_SELECT ) && SkillIsSelected( pGame, pFocusUnit, nSkillID ) ) ||
					( pSkillData->nUIThrobsOnState != INVALID_ID && UnitHasState( pGame, pFocusUnit, pSkillData->nUIThrobsOnState ) ) )
				{
					static const float fPeriodMS = 1000.0f;
					float fTimeRatio = FMODF((float)AppCommonGetCurTime(), fPeriodMS) / fPeriodMS;
					float fFading = fabs(1.0f - (fTimeRatio * 2.0f));

					UIComponentSetFade(pHighlight, fFading);
					bShowHighlight = TRUE;
				}
			}
		}

		// check for can't use item
		UNIT *player = UIGetControlUnit();
		if (player && item && ItemIsUsable( player, item, NULL, ITEM_USEABLE_MASK_NONE ) != USE_RESULT_OK)
		{
			float fScale = 1.0f;
			BOOL bIconIsRed = FALSE;
			//SkillSetTarget( player, UnitGetData(item)->nSkillWhenUsed, INVALID_ID, UnitGetId( item ) );
			c_SkillGetIconInfo( pFocusUnit, UnitGetData(item)->nSkillWhenUsed, bIconIsRed, fScale, nTicksRemaining, TRUE );
			if( fScale != 1 &&
				pCooldownHighlight )
			{

				float fWidth = pCooldownHighlight->m_fWidth;// * UIGetScreenToLogRatioX(pCooldownHighlight->m_bNoScale);
				float fHeight = pCooldownHighlight->m_fHeight;// * UIGetScreenToLogRatioY(pCooldownHighlight->m_bNoScale);
				UI_GFXELEMENT* pElement = UIComponentAddBoxElement(pCooldownHighlight, 0.0f, 
					fHeight * fScale, fWidth, fHeight, NULL, UI_MAKE_COLOR(192, UI_HILITE_GRAY));
				if( AppIsTugboat() && pElement )
				{
					pElement->m_fZDelta = -.0002f;
				}
			}
			else
			{
				bGrayOut = TRUE;
			}
		}

		// if a cooldown is in effect for this slot
		if (nTicksRemaining > 0)
		{
			if( AppIsHellgate() )
			{
				bGrayOut = TRUE;
			}

			// display time left
			WCHAR szBuff[32] = { L"0" };
			//PIntToStr(szBuff, 32, (nCooldownTicks + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND);

			int nDurationSeconds = (nTicksRemaining + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND;
			LanguageGetDurationString(szBuff, arrsize(szBuff), nDurationSeconds);

			UILabelSetText(pCooldown, szBuff, TRUE);

		}
		else
		{
			UILabelSetText(pCooldown, L"", TRUE);
		}

		// we don't need to do this anymore if we're repainting every frame
		//// register another draw
		//if (nCooldownTicks ||
		//	(pActiveSlot->m_nStateActive != INVALID_LINK &&
		//	pActiveSlot->m_wStateActiveIndex != INVALID_LINK))
		//{
		//	if (pActiveSlot->m_tMainAnimInfo.m_dwAnimTicket)
		//	{
		//		CSchedulerCancelEvent(pActiveSlot->m_tMainAnimInfo.m_dwAnimTicket);
		//	}
		//	pActiveSlot->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime()+MSECS_PER_GAME_TICK, pActiveSlot, UIMSG_PAINT, 0, 0);
		//}
				
		// Draw quantities (for consumables)
		if (pQuantity)
		{
			if (item && UnitGetSkillID(item) == INVALID_ID)
			{
				int nQuantity = ItemGetQuantity( item );
				int nClass = UnitGetClass( item );
				if (tag->m_nLastUnittype != INVALID_LINK)
				{
					const UNIT_DATA *pUnitData = UnitGetData(item);
					ASSERT_RETVAL(pUnitData, UIMSG_RET_NOT_HANDLED);
					UNIT *pOtherItem = NULL;
					DWORD dwFlags = 0;
					SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
					while ((pOtherItem = UnitInventoryLocationIterate(pFocusUnit, pUnitData->m_nRefillHotkeyInvLoc, pOtherItem, dwFlags)) != NULL)
					{
						if (UnitGetClass( pOtherItem ) == nClass &&
							pOtherItem != item)
						{
							nQuantity += ItemGetQuantity( pOtherItem );
						}
					}
				}

				LanguageFormatIntString( szQuantity, 32, nQuantity );

			}
		}

		if (bGrayOut && pCooldownHighlight  )
			UIComponentAddPrimitiveElement(pCooldownHighlight, UIPRIM_BOX, 0, UI_RECT(0.0f, 0.0f, pCooldownHighlight->m_fWidth , pCooldownHighlight->m_fHeight ), NULL, NULL, UI_MAKE_COLOR(128, UI_HILITE_GRAY));
	}
	else
	{
		UILabelSetText(pCooldown, L"");
	}

	UIComponentSetVisible(pHighlight, bShowHighlight);

	if (pQuantity)
	{
		UILabelSetText(pQuantity, szQuantity, TRUE);
	}

	// if the mouse is over this slot
	if (msg != UIMSG_MOUSELEAVE && 
		UIComponentCheckBounds(pActiveSlot))
	{
		// if there is a something in the mouse
		// or something in the slot
		if (g_UI.m_Cursor->m_idUnit != INVALID_ID ||
			 sUnitCanGoInActiveBar(UIGetCursorUnit()) ||
			(tag && (tag->m_idItem[0] != INVALID_ID || tag->m_idItem[1] != INVALID_ID)))
		{
			if (pActiveSlot->m_pLitFrame)
			{
				if( AppIsHellgate() )
				{
					UIComponentAddElement(pActiveSlot, NULL, pActiveSlot->m_pLitFrame, UI_POSITION(0.0f, 0.0f));
				}
				else
				{
	
					UIComponentAddElement(pActiveSlot, NULL, pActiveSlot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_WHITE, NULL, FALSE, .5f, .5f);
				}
			}
		}
	}
	//return msg != UIMSG_PAINT && msg != UIMSG_INVENTORYCHANGE && msg != UIMSG_SETCONTROLUNIT;	//return 1 for "handled" unless this is a Paint message (which needs to continue around).
	return ( msg != UIMSG_SETCONTROLSTAT ) ? UIMSG_RET_HANDLED : UIMSG_RET_HANDLED_END_PROCESS;//UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIActiveSlotOnPaint(component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (!UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_ACTIVESLOT *pActiveSlot = UICastToActiveSlot(component);

	UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(pActiveSlot->m_nSlotNumber);

	if (tag)
	{
		if (tag->m_idItem[0] != INVALID_ID)
		{
			UNIT *pUnit = UnitGetById(AppGetCltGame(), tag->m_idItem[0]);
			UISetHoverTextItem(component, pUnit);
			UISetHoverUnit(UnitGetId(pUnit), UIComponentGetId(component));
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		else if (tag->m_nSkillID[0] != INVALID_ID)
		{
			UISetHoverTextSkill(component, UIGetControlUnit(), tag->m_nSkillID[0]);
		}
	}

	return UIMSG_RET_HANDLED;
}

////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//UI_MSG_RETVAL UIActiveSlotShowBoost(
//	UI_COMPONENT* component,
//	int msg,
//	DWORD wParam,
//	DWORD lParam)
//{
//	ASSERT(lParam);		// This should be here or we'll get leaks
//
//	UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)CAST_TO_VOIDPTR(lParam);
//
//	// If it's a boost
//	//if (pData->m_bClearing /* || pData->m_nDuration*/)
//	{
//		UI_ACTIVESLOT *pActiveSlot = UICastToActiveSlot(component);
//
//		if (pData->m_bClearing)
//		{
//			if (pActiveSlot->m_nStateActive == pData->m_nState && pActiveSlot->m_wStateActiveIndex == pData->m_wStateIndex)
//			{
//				pActiveSlot->m_nStateActive = INVALID_LINK;
//				pActiveSlot->m_wStateActiveIndex = (WORD)INVALID_LINK;
//				UIComponentHandleUIMessage(pActiveSlot, UIMSG_PAINT, 0, 0);
//			}
//		}
//		else
//		{
//			UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(pActiveSlot->m_nSlotNumber);
//
//			// if this state was set by the skill of this slot
//			if (tag && tag->m_nSkillID[0] != INVALID_ID && tag->m_nSkillID[0] == pData->m_nSkill)
//			{
//				int nSkillID = tag->m_nSkillID[0];
//				if (nSkillID != INVALID_ID)
//				{ /// this is not the proper way to get a skill's cooldown.  Use SkillGetIconInfo()  
//					GAME *pGame = AppGetCltGame();
//					pActiveSlot->m_nStateActive = pData->m_nState;
//					pActiveSlot->m_wStateActiveIndex = pData->m_wStateIndex;
//					pActiveSlot->m_nStateActiveDuration = pData->m_nDuration;
//					const SKILL_DATA* pSkillData = SkillGetData(pGame, nSkillID);
//					int nCooldownTicks = SkillGetCooldown(pGame, UIGetControlUnit(), NULL, nSkillID, pSkillData, 0);
//					pActiveSlot->m_dwTickCooldownOver = GameGetTick(pGame) + nCooldownTicks;
//					
//				}
//				UIComponentHandleUIMessage(pActiveSlot, UIMSG_PAINT, 0, 0);
//			}
//		}
//	}
//	return UIMSG_RET_HANDLED;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UISetHoverUnit(INVALID_ID, INVALID_ID);
	UIClearHoverText();
	UIActiveSlotOnPaint(component, UIMSG_PAINT, 0, 0);

	UI_ACTIVESLOT* pSlot = UICastToActiveSlot(component);

	UIActiveBarHotkeyUp(AppGetCltGame(), UIGetControlUnit(), pSlot->m_nSlotNumber);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_ACTIVESLOT* pSlot = UICastToActiveSlot(component);
	UNITID idCursorUnit = UIGetCursorUnit();
	int		nCursorSkill = UIGetCursorSkill();

	if (idCursorUnit != INVALID_ID &&
		!sUnitCanGoInActiveBar(idCursorUnit))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (nCursorSkill != INVALID_ID &&
		!sSkillCanGoInActiveBar(nCursorSkill))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(pSlot->m_nSlotNumber);

	UNITID idOldUnit1 = INVALID_ID;
	UNITID idOldUnit2 = INVALID_ID;
	int    nOldSkillID1 = INVALID_ID;
	int	   nOldSkillID2 = INVALID_ID;

	if (tag)	// save what was in the slot so we can put it in the mouse afterward
	{
		idOldUnit1 = tag->m_idItem[0];
		idOldUnit2 = tag->m_idItem[1];
		nOldSkillID1 = tag->m_nSkillID[0];
		nOldSkillID2 = tag->m_nSkillID[1];
	}

	MSG_CCMD_HOTKEY tMsg;
	tMsg.idItem[0] = idCursorUnit;
	tMsg.idItem[1] = INVALID_ID;
	tMsg.nSkillID[0] = nCursorSkill;
	tMsg.nSkillID[1] = INVALID_ID;
	tMsg.bHotkey = (BYTE)pSlot->m_nSlotNumber;
	c_SendMessage(CCMD_HOTKEY, &tMsg);

	UNIT * pPlayer = UIGetControlUnit();
	if (tMsg.idItem[0] != INVALID_ID)
	{
		UIPlayItemPutdownSound(UnitGetById(AppGetCltGame(), tMsg.idItem[0]), pPlayer, INVALID_LINK);
	}
	else if (tMsg.idItem[1] != INVALID_ID)
	{
		UIPlayItemPutdownSound(UnitGetById(AppGetCltGame(), tMsg.idItem[1]), pPlayer, INVALID_LINK);
	}

	if (idOldUnit1 != INVALID_ID)
	{
		UISetCursorUnit(idOldUnit1, TRUE, pSlot->m_nSlotNumber);	// put what we replaced in the cursor
		UIPlayItemPickupSound(UnitGetById(AppGetCltGame(), idOldUnit1));
	}
	else if (nOldSkillID1 != INVALID_ID)
	{
		UISetCursorSkill(nOldSkillID1, TRUE);
	}
	else
	{
		if (tMsg.nSkillID[0] != INVALID_ID)
		{
			int nSoundId = GlobalIndexGet(GI_SOUND_UI_SKILL_PUTDOWN);
			if (nSoundId != INVALID_LINK)
			{
				c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
			}
		}
		UISetCursorUnit(INVALID_ID, FALSE);
		UISetCursorSkill(INVALID_ID, TRUE);
	}

	UIComponentHandleUIMessage(pSlot, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnRButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_ACTIVESLOT* pSlot = UICastToActiveSlot(component);

	UIActiveBarHotkeyDown(AppGetCltGame(), UIGetControlUnit(), pSlot->m_nSlotNumber);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActiveSlotOnRButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UI_ACTIVESLOT* pSlot = UICastToActiveSlot(component);

	UIActiveBarHotkeyUp(AppGetCltGame(), UIGetControlUnit(), pSlot->m_nSlotNumber);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline UI_ACTIVESLOT *sUIGetActiveSlot( 
	int nSlot)
{
	// First find the right slot
	UI_COMPONENT* pActiveBar = UIComponentGetByEnum(UICOMP_ACTIVEBAR);


	while (pActiveBar)
	{
		UI_COMPONENT *pChild = pActiveBar->m_pFirstChild;
		while(pChild)
		{
			if (pChild->m_eComponentType == UITYPE_ACTIVESLOT)
			{
				UI_ACTIVESLOT* pSlot = UICastToActiveSlot(pChild);
				if (pSlot->m_nSlotNumber == nSlot)	
				{
					return pSlot;
				}
			}
			pChild = pChild->m_pNextSibling;
		}

		if (pActiveBar == UIComponentGetByEnum(UICOMP_ACTIVEBAR))
		{
			// now check the other active bar
			pActiveBar = UIComponentGetByEnum(UICOMP_RTS_ACTIVEBAR);
		}
		else
		{
			pActiveBar = NULL;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIActiveBarHotkeyUp(
	GAME* game,
	UNIT* unit,
	int nSlotNumber)
{
    UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(nSlotNumber);

	if (!tag)
	{
		return;
	}

    if (tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID)
    {
		int nSkillID = (tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1]);
	    SkillStopRequest(UIGetControlUnit(), nSkillID, TRUE, TRUE);
//	    trace("  Stopping skill id %d\n", nSkillID);
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIActiveBarHotkeyDown(
	GAME* game,
	UNIT* unit,
	int nSlotNumber,
	VECTOR* vClickLocation /*= NULL*/ )
{
    UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(nSlotNumber);

    if (!tag)
	{
		return;
	}

	if (tag->m_idItem[0] != INVALID_ID || tag->m_idItem[1] != INVALID_ID)
    {
	    UNIT *pUnit = UnitGetById(AppGetCltGame(), (tag->m_idItem[0] != INVALID_ID ? tag->m_idItem[0] : tag->m_idItem[1]));
	    ASSERT_RETURN(pUnit);

	    int nLocation = INVLOC_NONE;
	    int nSkillID = UnitGetSkillID(pUnit);
	    if ( nSkillID != INVALID_ID )
	    {
		    c_SkillControlUnitRequestStartSkill( game, nSkillID );
//		    trace("Starting skill id %d\n", nSkillID);
	    }
	    else if (ItemHasSkillWhenUsed(pUnit) || ItemIsQuestActiveBarOk(UIGetControlUnit(), pUnit))
		{
			if( UnitDataTestFlag(UnitGetData(pUnit), UNIT_DATA_FLAG_ITEM_IS_TARGETED) &&
				vClickLocation )
			{				

				c_PlayerMouseAimLocation( game, vClickLocation );

				c_SendUnitSetFaceDirection( UIGetControlUnit(), UnitGetFaceDirection( UIGetControlUnit(), FALSE ) );

				UnitSetStatVector(UnitGetUltimateOwner(pUnit), STATS_SKILL_TARGET_X, UnitGetData(pUnit)->nSkillWhenUsed, 0, *vClickLocation );
			}
			// CTRL-clicking activeslots uses items on hirelings
			if( AppIsTugboat() && (GetKeyState(VK_CONTROL) & 0x8000) &&
				UnitIsA( pUnit, UNITTYPE_POTION ) &&
				!(UnitDataTestFlag(UnitGetData(pUnit), UNIT_DATA_FLAG_ITEM_IS_TARGETED)))
			{
				// look for a hireling
				UNIT* pHireling =  NULL;

				UNIT * pItem = NULL;
				while ((pItem = UnitInventoryIterate(unit, pItem)) != NULL)		// might want to narrow this iterate down if we can
				{
					if (PetIsPet(pItem) &&  UnitDataTestFlag(UnitGetData(pItem), UNIT_DATA_FLAG_SHOWS_PORTRAIT) &&
						UnitIsA( pItem, UNITTYPE_HIRELING ) )
					{

						if (IsUnitDeadOrDying( pItem ) || UnitGetOwnerUnit( pItem ) != unit)
						{
							continue;
						}
						pHireling = pItem;
						break;
					}
				}
				if( pHireling )
				{
					USE_RESULT eUseResult = UnitIsUsable( pHireling, pUnit );
					if (eUseResult == USE_RESULT_OK)
					{
						// Use the unit
						c_ItemUseOn(pUnit, pHireling);
						UI_ACTIVESLOT *pSlotComponent = sUIGetActiveSlot(nSlotNumber);
						if (pSlotComponent)
						{
							UIComponentHandleUIMessage(pSlotComponent, UIMSG_PAINT, 0, 0);			
						}
					}
					else
					{
						c_ItemCannotUse( pUnit, eUseResult );
					}
				}
			}
			else
			{
				const SKILL_DATA * pSkillData = SkillGetData( game, UnitGetData(pUnit)->nSkillWhenUsed );

				if ( pSkillData &&
					SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAYER_STOP_MOVING ) )
				{
					c_PlayerStopPath( game );
					c_PlayerMouseAim( game, TRUE );
				}
				USE_RESULT eUseResult = UnitIsUsable( UIGetControlUnit(), pUnit );
				if (eUseResult == USE_RESULT_OK)
				{
					// Use the unit
					c_ItemUse(pUnit);
					UI_ACTIVESLOT *pSlotComponent = sUIGetActiveSlot(nSlotNumber);
					if (pSlotComponent)
					{
						UIComponentHandleUIMessage(pSlotComponent, UIMSG_PAINT, 0, 0);			
					}
				}
				else
				{
					c_ItemCannotUse( pUnit, eUseResult );
				}
			}


	    }
	    else
	    {
		    UNIT *pPlayerUnit = UIGetControlUnit();
		    ASSERT(pPlayerUnit);

		    if (tag->m_idItem[0] != INVALID_ID && tag->m_idItem[1] != INVALID_ID)
		    {
			    if (InventoryDoubleEquip(AppGetCltGame(), pPlayerUnit, tag->m_idItem[0], tag->m_idItem[1], INVLOC_RHAND, INVLOC_LHAND))
			    {
//					UISetCursorUnit(INVALID_ID, TRUE);
				    UIPlayItemPutdownSound(pUnit, pPlayerUnit, nLocation);
			    }
		    }
		    else
		    {
			    UIAutoEquipItem(pUnit, pPlayerUnit);
		    }
	    }
	}
	else if (tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID)
	{
		int nSkillID = (tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1]);
	    c_SkillControlUnitRequestStartSkill( game, nSkillID );
//	    trace("Starting skill id %d\n", nSkillID);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_TB_HotSpellHotkeyUp(
	GAME* game,
	UNIT* unit,
	int n)
{

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UI_TB_HotSpellHotkeyDown(
	GAME* game,
	UNIT* unit,
	int n)
{
	
	
	
	
	UNIT_TAG_HOTKEY* tag = UIUnitGetHotkeyTag(n);

	UI_COMPONENT *pSpellsScreen = UIComponentGetByEnum(UICOMP_SPELLHOTBAR);
	ASSERT_RETZERO(pSpellsScreen);
	//find the spellbookgrid
	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	UI_POSITION skillpos;
	if( !UIComponentGetActive(pSpellsScreen) )
	{
		pSpellsScreen = UIComponentGetByEnum(UICOMP_SPELLHOTBARLEFT);
		if( !pSpellsScreen || !UIComponentGetActive(pSpellsScreen) )
		{
			pSpellsScreen = NULL;
		}
	}
	
	if( pSpellsScreen )
	{
		UI_COMPONENT *pComponent = UIComponentFindChildByName(pSpellsScreen, "skills grid 4");
		if( !UIComponentCheckBounds(pComponent, x, y, &pos) ||
			!UIComponentGetActive(pComponent) )
		{
			pSpellsScreen = NULL;
		}
	}

	if( !pSpellsScreen )
	{
		pSpellsScreen = UIComponentGetByEnum(UICOMP_PANEMENU_SKILLS);
		if( !UIComponentGetActive(pSpellsScreen) ||
			!UIComponentCheckBounds(pSpellsScreen, x, y, &pos) )
		{
			pSpellsScreen = NULL;
		}
	}


	if( pSpellsScreen )
	{
		UI_COMPONENT *pComponent = UIComponentFindChildByName(pSpellsScreen, "skills grid 1");
		
		if ( !pComponent ||
			!UIComponentCheckBounds(pComponent, x, y, &pos) ||
			!UIComponentGetActive(pComponent))
		{
			pComponent = UIComponentFindChildByName(pSpellsScreen, "skills grid 2");
		}
		if (!pComponent ||
			!UIComponentCheckBounds(pComponent, x, y, &pos) ||
			!UIComponentGetActive(pComponent))
		{
			pComponent = UIComponentFindChildByName(pSpellsScreen, "skills grid 3");
		}
		if (!pComponent ||
			!UIComponentCheckBounds(pComponent, x, y, &pos) ||
			!UIComponentGetActive(pComponent))
		{
			pComponent = UIComponentFindChildByName(pSpellsScreen, "skills grid 4");
		}
		UI_SKILLSGRID *pGrid = pComponent ? UICastToSkillsGrid( pComponent ) : NULL;

		if ( pGrid )
		{
			x -= pos.m_fX;
			y -= pos.m_fY;
			// we need logical coords for the element comparisons ( getskillatpos )
			float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
			float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);

			int Slot = 1;
			if(  pGrid->m_bIsHotBar )
			{
				Slot = (int)pGrid->m_dwParam;
			}
			int nSkillID = sSkillsGridGetSkillAtPosition(pGrid, logx, logy, pos );
			if( sSkillCanGoInActiveBar( nSkillID ) )
			{
				BOOL bSameSlot = FALSE;
				// anything currently there? and if so, do we already have this mapped somewhere else? Need to swap.
				if( tag )
				{
					//UNITID idOldUnit1 = tag->m_idItem[0];

					for( int t = TAG_SELECTOR_SPELLHOTKEY1; t <= TAG_SELECTOR_SPELLHOTKEY12; t++ )
					{
						UNIT_TAG_HOTKEY* oldtag = UIUnitGetHotkeyTag(t);
						if( oldtag && 
							( oldtag->m_nSkillID[Slot] != INVALID_ID &&
							    oldtag->m_nSkillID[Slot] == nSkillID ) )
						{
							MSG_CCMD_HOTKEY msg;
							msg.idItem[0] = INVALID_ID;
							msg.idItem[1] = INVALID_ID;
							msg.nSkillID[0] = INVALID_ID;
							msg.nSkillID[1] = INVALID_ID;
							msg.bHotkey = (BYTE)t;
							c_SendMessage(CCMD_HOTKEY, &msg);
							if( oldtag == tag )
							{
								bSameSlot = TRUE;
							}
						}
					}
				}

				

				if( nSkillID != INVALID_ID && !bSameSlot )
				{
					MSG_CCMD_HOTKEY msg;
					msg.idItem[0] = INVALID_ID;
					msg.idItem[1] = INVALID_ID;
					msg.nSkillID[0] = INVALID_ID;
					msg.nSkillID[1] = INVALID_ID;
					msg.nSkillID[Slot] = nSkillID;

					
					msg.bHotkey = (BYTE)n;
					c_SendMessage(CCMD_HOTKEY, &msg);
					return TRUE;
				}
			}
		}

	}

	const GLOBAL_DEFINITION * globals = DefinitionGetGlobal();
	ASSERT(globals);
	if( globals && (globals->dwFlags & GLOBAL_FLAG_CAST_ON_HOTKEY) )
	{
		return FALSE;
	}

	if (tag && 
		(tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID))
	{

		int nSkillID = ( tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1] );
		int Slot = tag->m_nSkillID[0] != INVALID_ID ? 0 : 1;
		int SkillSlot = Slot ? STATS_SKILL_RIGHT : STATS_SKILL_LEFT;
		if (nSkillID != INVALID_ID &&
			sSkillCanGoInActiveBar( nSkillID ) )
		{

			int nPrevSkill = UnitGetStat( UIGetControlUnit(), SkillSlot );
			// if we've got an old skill on the hotkey currnetly active, DON'T DO IT!
			// address later
			if( nPrevSkill != nSkillID )
			{
				if( nPrevSkill != INVALID_ID )
				{
					SkillStopRequest(UIGetControlUnit(), nPrevSkill, TRUE, FALSE);
					/*int nTicksToHold = sSkillNeedsToHold( UIGetControlUnit(), NULL, nPrevSkill );
					if( nTicksToHold > 0 )
					{
						return;
					}*/
				}
				static int nSkillSelectSound = INVALID_LINK;
				if (nSkillSelectSound == INVALID_LINK)
				{
					nSkillSelectSound = GlobalIndexGet( GI_SOUND_SKILL_SELECT );
				}
				if (nSkillSelectSound != INVALID_LINK)
				{
					c_SoundPlay( nSkillSelectSound );
				}
			}
			if( nSkillID != INVALID_ID )
			{
				//SkillSelect( game, UIGetControlUnit(), nSkillID );
				UnitSetStat( UIGetControlUnit(), SkillSlot, nSkillID);
				c_SendSkillSaveAssign((WORD)nSkillID, (BYTE)Slot );
			}
			
			return TRUE;
		}


	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInstanceComboOnSelect( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{

	UI_COMPONENT * pInstanceCombo = UIComponentGetByEnum(UICOMP_TOWN_INSTANCE_COMBO);
	UI_COMBOBOX * pCombo = UICastToComboBox(pInstanceCombo);
	int nIndex = UIComboBoxGetSelectedIndex(pCombo);

	if (nIndex == INVALID_ID)
	{
		return UIMSG_RET_HANDLED;
	}

	QWORD nZoneIndexData = UIComboBoxGetSelectedData(pCombo);

	MSG_CCMD_WARP_TO_GAME_INSTANCE message;
	message.nTypeOfInstance = GAME_INSTANCE_TOWN;
	message.idGameToWarpTo = (GAMEID)nZoneIndexData;
	
	c_SendMessage( CCMD_WARP_TO_GAME_INSTANCE, &message );

	
	return UIMSG_RET_HANDLED;
}

#define MAX_PARTY_STRING	256
struct UI_PARTY_MEMBER
{
	WCHAR			m_szName[MAX_PARTY_STRING];
	int				m_nPlayerUnitClass;
	int				m_nLevel;
	int				m_nRank;
	PGUID			m_idPlayer;
};

struct UI_PARTY_DESC
{
	int		m_nPartyID;
	WCHAR	m_szPartyName[MAX_PARTY_STRING];
	WCHAR	m_szPartyDesc[MAX_PARTY_STRING];

	int		m_nPlayersRecieved;
	int		m_nCurrentPlayers;
	int		m_nMaxPlayers;
	int		m_nMinLevel;
	int		m_nMaxLevel;
	BOOL	m_bForPlayerAdvertisementOnly;
	int		m_nMatchType;

	GAMEID	m_idGame;				// For PVP games only
	int		m_nInstanceNumber;		// For PVP games only
	int		m_nLevelDef;			// For PVP games only

	UI_PARTY_MEMBER		m_tPartyMembers[MAX_CHAT_PARTY_MEMBERS];

	UI_GFXELEMENT *m_pNameElement;
	UI_GFXELEMENT *m_pDescElement;
	UI_GFXELEMENT *m_pSizeElement;
	UI_GFXELEMENT *m_pTypeElement;
	UI_GFXELEMENT *m_pPlayersElement;
};

static int				gnPartyListCount = 0;
static int				gnMatchListCount = 0;
static UI_PARTY_DESC *	gpUIPartyList = NULL;
static UI_PARTY_DESC *	gpUIMatchList = NULL;

static UI_PARTY_DESC * sGetCurrentPartyDesc()
{
	UI_COMPONENT *pPartyList = UIComponentGetByEnum(UICOMP_PARTYLIST);
	if (pPartyList)
	{
		if ((int)pPartyList->m_dwData >= 0 && gpUIPartyList)
		{
			for (int i=0; i<gnPartyListCount; ++i)
			{
				if (gpUIPartyList[i].m_nPartyID == (int)pPartyList->m_dwData)
				{
					return &(gpUIPartyList[i]);
				}
			}
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSortPartyList(
	void * pContext,
	const void * pLeft,
	const void * pRight)
{
	int nSortColumn = *(int *)pContext;

	UI_PARTY_DESC *pParty1 = (UI_PARTY_DESC *)pLeft;
	UI_PARTY_DESC *pParty2 = (UI_PARTY_DESC *)pRight;

	switch (nSortColumn)
	{
		case 0: 
		{
			return PStrICmp(pParty1->m_szPartyName, pParty2->m_szPartyName, arrsize(pParty1->m_szPartyName)); 
		}
		case 1:
		{
			return PStrICmp(pParty1->m_szPartyDesc, pParty2->m_szPartyDesc, arrsize(pParty1->m_szPartyDesc)); 
		}
	};

	ASSERTV_RETZERO(0, "invalid sort column %d passed to sSortPartyList", nSortColumn);
	return 0; 
}

static int sPartyListSortColumn = 0;

static void sUIPartyListSort(
	int nColumn = -1)
{
	if (nColumn >= 0)
	{
		if (sPartyListSortColumn == nColumn)
		{
			return;
		}
		sPartyListSortColumn = nColumn;
	}

	qsort_s(gpUIPartyList, gnPartyListCount, sizeof(UI_PARTY_DESC), sSortPartyList, (void *)&sPartyListSortColumn);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIPartyListAddPartyDesc(
	LPVOID pArray,
	DWORD partyCount )
{
	CHAT_RESPONSE_MSG_PARTY_INFO * partyArray = (CHAT_RESPONSE_MSG_PARTY_INFO*)pArray;
	UI_COMPONENT *pPartyList = UIComponentGetByEnum(UICOMP_PARTYLIST);

	gpUIPartyList = (UI_PARTY_DESC*)REALLOCZ(NULL, gpUIPartyList, sizeof(UI_PARTY_DESC) * partyCount);
	gnPartyListCount = partyCount;

	BOOL bPartyFound = FALSE;

	for (UINT ii = 0; ii < partyCount; ++ii)
	{
		UI_PARTY_DESC * tParty = &gpUIPartyList[ii];
		tParty->m_nPartyID = partyArray[ii].PartyInfo.IdPartyChannel;
		PStrCopy(tParty->m_szPartyName, partyArray[ii].PartyInfo.wszLeaderName, MAX_PARTY_STRING);
		PStrCopy(tParty->m_szPartyDesc, partyArray[ii].PartyInfo.wszPartyDesc, MAX_PARTY_STRING);
		tParty->m_nCurrentPlayers = partyArray[ii].PartyInfo.wCurrentMembers;
		tParty->m_nMaxPlayers = partyArray[ii].PartyInfo.wMaxMembers;
		tParty->m_nMinLevel = partyArray[ii].PartyInfo.wMinLevel;
		tParty->m_nMaxLevel = partyArray[ii].PartyInfo.wMaxLevel;
		tParty->m_bForPlayerAdvertisementOnly = partyArray[ii].PartyInfo.bForPlayerAdvertisementOnly;
		tParty->m_nMatchType = 0; partyArray[ii].PartyInfo.nMatchType;

		if ((int)pPartyList->m_dwData == tParty->m_nPartyID)
		{
			bPartyFound = TRUE;
		}		
	}

	if (!bPartyFound && s_CurrentPlayerMatchMode != PMM_MATCH)
	{
		pPartyList->m_dwData = (DWORD)INVALID_ID;
	}

	int oldSortColumn = sPartyListSortColumn;
	sPartyListSortColumn = -1;
	sUIPartyListSort(oldSortColumn);

	UI_COMPONENT *pPartyPanel = UIComponentGetByEnum(UICOMP_PARTYPANEL);
	if (pPartyPanel)
	{
		UIPartyPanelContextChange(pPartyPanel);
	}

	if (UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL))
	{
		UIPlayerMatchDialogContextChange();
	}

	UIComponentHandleUIMessage(pPartyList, UIMSG_PAINT, 0, 0);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIPartyListAddPartyMember(
	int	nPartyID,
	const WCHAR * szName,
	int nLevel,
	int nRank,
	int nPlayerUnitClass,
	PGUID idPlayer)
{
	BOOL success = FALSE;
	UI_PARTY_DESC *pParty = gpUIPartyList;
	int i = 0;
	while (pParty && i < gnPartyListCount)
	{
		if (pParty->m_nPartyID == nPartyID)
		{
			if (pParty->m_nPlayersRecieved >= pParty->m_nMaxPlayers ||
				pParty->m_nPlayersRecieved >= MAX_CHAT_PARTY_MEMBERS ||
				pParty->m_nPlayersRecieved >= pParty->m_nCurrentPlayers)
			{
				break;
			}

			PStrCopy(pParty->m_tPartyMembers[pParty->m_nPlayersRecieved].m_szName, szName, MAX_PARTY_STRING);
			pParty->m_tPartyMembers[pParty->m_nPlayersRecieved].m_nPlayerUnitClass = nPlayerUnitClass;
			pParty->m_tPartyMembers[pParty->m_nPlayersRecieved].m_nLevel = nLevel;
			pParty->m_tPartyMembers[pParty->m_nPlayersRecieved].m_nRank = nRank;
			pParty->m_tPartyMembers[pParty->m_nPlayersRecieved].m_idPlayer = idPlayer;
			pParty->m_nPlayersRecieved++;

			success = TRUE;
			break;
		}

		pParty++;
		i++;
	}

	UI_COMPONENT *pMembers = UIComponentGetByEnum(UICOMP_PARTYMEMBERS_PANEL);
	if (pMembers)
		UIComponentHandleUIMessage(pMembers, UIMSG_PAINT, 0, 0);

	return success;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPartyListClose()
{
	gnPartyListCount = 0;
	FREE(NULL, gpUIPartyList);
	gpUIPartyList = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT* pParent = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_PARTYLIST_TAB) : component->m_pParent;
	if (!pParent)
	{
		return UIMSG_RET_NOT_HANDLED; // possible it hasn't loaded yet
	}

	UIComponentRemoveAllElements(component);

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
	ASSERT_RETVAL(pFont, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pTabs = UIComponentFindChildByName(pParent, "party list tabs");
	ASSERT_RETVAL(pTabs, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pLabelPartyName	= UIComponentFindChildByName(pTabs, "label party name");
	if (!pLabelPartyName)
		pLabelPartyName	= UIComponentFindChildByName(pTabs, "label leader name");
	ASSERT_RETVAL(pLabelPartyName, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pLabelPartyDesc	= UIComponentFindChildByName(pTabs, "label party description");
	ASSERT_RETVAL(pLabelPartyDesc, UIMSG_RET_NOT_HANDLED);


	UI_COMPONENT* pHilightBar = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_PARTYLIST_HILIGHT) : UIComponentFindChildByName(component->m_pParent, "party list highlight");
	ASSERT_RETVAL(pHilightBar, UIMSG_RET_NOT_HANDLED);
	if (pHilightBar)
	{
		pHilightBar->m_ScrollPos.m_fY = component->m_ScrollPos.m_fY;
		UIComponentSetVisible(pHilightBar, FALSE);
	}
	UI_COMPONENT *pShowPartiesButton = UIComponentFindChildByName(pParent, "show parties btn");
	UI_COMPONENT *pShowIndividualsButton = UIComponentFindChildByName(pParent, "show individuals btn");

	//const WCHAR *szPartyHeaderString = GlobalStringGet( GS_UI_PARTYLIST_PARTY );
	//const WCHAR *szDescHeaderString  = GlobalStringGet( GS_UI_PARTYLIST_DESCRIPTION );
	//const WCHAR *szCountHeaderString = GlobalStringGet( GS_UI_PARTYLIST_COUNT );

	//UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	//UI_TEXTURE_FRAME *pHighlightBar = NULL;
	//if (pTexture)		
	//	pHighlightBar = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, "black_box");

	int nFontSize = UIComponentGetFontSize(component, pFont);
//	fFontsize *= UIGetScreenToLogRatioX(component->m_bNoScaleFont);

	UI_RECT rectNameCol(pLabelPartyName->m_Position.m_fX,	0.0f, pLabelPartyName->m_Position.m_fX + pLabelPartyName->m_fWidth,	component->m_fHeight);
	UI_RECT rectDescCol(pLabelPartyDesc->m_Position.m_fX,	0.0f, pLabelPartyDesc->m_Position.m_fX + pLabelPartyDesc->m_fWidth,	component->m_fHeight);

	// give a litte room between the columns
//	rectNameCol.m_fX2 -= 5.0f;
//	rectDescCol.m_fX2 -= 5.0f;

	UI_POSITION pos;
	pos.m_fY = 0.0f;
	UI_PARTY_DESC *pParty = gpUIPartyList;
	UI_SIZE size;
	size.m_fHeight = (float)nFontSize; // * UIGetScreenToLogRatioY(component->m_bNoScaleFont);
	int i = 0;
	const float fLineSpacing = (nFontSize * UIGetScreenToLogRatioY(component->m_bNoScaleFont));
	UI_TEXTURE_FRAME* pPartyFrame = NULL;
	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if( AppIsHellgate() )
	{
		pPartyFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "party_icon_sm");
	}
	else
	{
		pPartyFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "button_partyfunc");
	}

	while (pParty && i < gnPartyListCount)
	{
		if (pParty->m_bForPlayerAdvertisementOnly)
		{
			if (pShowIndividualsButton && !UIButtonGetDown(UICastToButton(pShowIndividualsButton)))
			{
				goto _NEXTROW;
			}
		}
		else
		{
			if (pShowPartiesButton && !UIButtonGetDown(UICastToButton(pShowPartiesButton)))
			{
				goto _NEXTROW;
			}
		}

		if (pParty->m_nPartyID != INVALID_ID)
		{
			DWORD dwFontColor = GFXCOLOR_WHITE;

			if (!pParty->m_bForPlayerAdvertisementOnly)
			{

				if (texture)
				{
					if( AppIsHellgate() )
					{
						UI_POSITION posIcon = pos;
						posIcon.m_fX = -(12.0f / UIGetScreenToLogRatioX(component->m_bNoScaleFont));
						posIcon.m_fY -= (4.0f / UIGetScreenToLogRatioX(component->m_bNoScaleFont));
						UIComponentAddElement(component, texture, pPartyFrame, posIcon);
					}
					else
					{
						UI_POSITION posIcon = pos;
						posIcon.m_fX = -(6.0f / UIGetScreenToLogRatioX(component->m_bNoScaleFont) * g_UI.m_fUIScaler);
						posIcon.m_fY -= (3.0f / UIGetScreenToLogRatioX(component->m_bNoScaleFont) * g_UI.m_fUIScaler);
						UIComponentAddElement(component, texture, pPartyFrame, posIcon, GFXCOLOR_WHITE, &UI_RECT(-6,-3, component->m_fWidth, component->m_fHeight));
					}
				}
				dwFontColor = GetFontColor(FONTCOLOR_FSDARKORANGE);
			}

			// write party name
			pos.m_fX = ( rectNameCol.m_fX1 + 20.0f * g_UI.m_fUIScaler ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
			size.m_fWidth = ( rectNameCol.m_fX2 - rectNameCol.m_fX1 - 20.0f ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
			pParty->m_pNameElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, pParty->m_szPartyName, pos, dwFontColor, &rectNameCol, UIALIGN_LEFT, &size, FALSE);

			// write party description
			pos.m_fX = rectDescCol.m_fX1;
			size.m_fWidth = ( rectDescCol.m_fX2 - rectDescCol.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
			pParty->m_pDescElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, pParty->m_szPartyDesc, pos, dwFontColor, &rectDescCol, UIALIGN_LEFT, &size, FALSE);

			pos.m_fY += fLineSpacing;

			if (pParty->m_nPartyID == (int)component->m_dwData)
			{
				pParty->m_pNameElement->m_dwColor = 
				pParty->m_pDescElement->m_dwColor = GetFontColor(FONTCOLOR_FSORANGE);

				if (pHilightBar)
				{
					pHilightBar->m_Position.m_fY = pParty->m_pNameElement->m_Position.m_fY;
					UIComponentSetVisible(pHilightBar, TRUE);
				}
			}
		}

	_NEXTROW:
		pParty++;
		i++;
	}


	UI_COMPONENT *pScrollBar = UIComponentIterateChildren(AppIsHellgate() ? component : pParent, NULL, UITYPE_SCROLLBAR, FALSE);
	if (pScrollBar)
	{	
		UI_SCROLLBAR *pScroll = UICastToScrollBar(pScrollBar);
		pScroll->m_fMin = 0;
		pScroll->m_fMax = pos.m_fY - component->m_fHeight;
		if (pScroll->m_fMax < 0.0f)
		{
			pScroll->m_fMax = 0.0f;
			UIComponentSetVisible(pScroll, FALSE);
		}
		else
		{
			UIComponentSetActive(pScroll, TRUE);
		}
		component->m_fScrollVertMax = pScroll->m_fMax;
		component->m_ScrollPos.m_fY = MIN(component->m_ScrollPos.m_fY, component->m_fScrollVertMax);
	}
	

	if (pHilightBar && !UIComponentGetVisible(pHilightBar) && UIComponentGetVisible(component))
	{
		component->m_dwData = (DWORD)INVALID_ID;

		UI_COMPONENT *pPartyPanel = UIComponentGetByEnum(UICOMP_PARTYPANEL);
		if (pPartyPanel)
		{
			UI_COMPONENT *pButton = NULL;
			pButton = UIComponentFindChildByName(pPartyPanel, "invite button");
			if (pButton)
			{
				UIComponentActivate(pButton, FALSE);
				UIComponentSetVisible(pButton, TRUE);
			}

			pButton = UIComponentFindChildByName(pPartyPanel, "join button");
			if (pButton)
			{
				UIComponentActivate(pButton, FALSE);
				UIComponentSetVisible(pButton, TRUE);
			}

			pButton = UIComponentFindChildByName(pPartyPanel, "whisper button");
			if (pButton)
			{
				UIComponentActivate(pButton, FALSE);
				UIComponentSetVisible(pButton, TRUE);
			}
		}
	}

	//pos.m_fY = 0.0f - (nFontSize + 10.0f);
	//pos.m_fY += component->m_ScrollPos.m_fY;

	//// write party name header
	//pos.m_fX = rectNameCol.m_fX1;
	//UIComponentAddTextElement(component, NULL, pFont, nFontSize, szPartyHeaderString, pos, GFXCOLOR_WHITE, NULL, UIALIGN_TOPLEFT, NULL, FALSE);

	//// write party description header
	//pos.m_fX = rectDescCol.m_fX1;
	//UIComponentAddTextElement(component, NULL, pFont, nFontSize, szDescHeaderString, pos, GFXCOLOR_WHITE, NULL, UIALIGN_TOPLEFT, NULL, FALSE);

	//// write party size header
	//pos.m_fX = rectNumCol.m_fX1;
	//UIComponentAddTextElement(component, NULL, pFont, nFontSize, szCountHeaderString, pos, GFXCOLOR_WHITE, NULL, UIALIGN_TOPLEFT, NULL, FALSE);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static DWORD sgdwRefreshingPartyAdTicket = INVALID_ID;

#define PARTY_AD_REFRESH_RATE 15

static void sRefreshPartyAdList(
	GAME*,
	const CEVENT_DATA&,
	DWORD)
{
	if (sgdwRefreshingPartyAdTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(sgdwRefreshingPartyAdTicket);
	}

	//	TODO: parties can be classified by string search patterns, should be used eventually...
	CHAT_REQUEST_MSG_GET_ADVERTISED_PARTIES getPartiesMsg;
	c_ChatNetSendMessage(
		&getPartiesMsg,
		USER_REQUEST_GET_ADVERTISED_PARTIES );

	TIME time = CSchedulerGetTime() + (PARTY_AD_REFRESH_RATE * SCHEDULER_TIME_PER_SECOND);
	CEVENT_DATA nodata;
	sgdwRefreshingPartyAdTicket = CSchedulerRegisterEvent(
									time,
									sRefreshPartyAdList,
									nodata);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UIPartyRequestMembers(int nPartyID);

static DWORD sgdwRefreshingPartyMembersTicket = INVALID_ID;

#define PARTY_MEMBERS_REFRESH_RATE 6

static void sRefreshPartyMembersList(
	GAME*,
	const CEVENT_DATA&,
	DWORD)
{
	if (sgdwRefreshingPartyMembersTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(sgdwRefreshingPartyMembersTicket);
	}

	UI_COMPONENT *pPartyList = UIComponentGetByEnum(UICOMP_PARTYLIST);
	ASSERT_RETURN(pPartyList);

	UIPartyRequestMembers((int)pPartyList->m_dwData);

	TIME time = CSchedulerGetTime() + (PARTY_MEMBERS_REFRESH_RATE * SCHEDULER_TIME_PER_SECOND);
	CEVENT_DATA nodata;
	sgdwRefreshingPartyMembersTicket = CSchedulerRegisterEvent(
		time,
		sRefreshPartyMembersList,
		nodata);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_PARTY_DESC * sUIPartyPanelGetMyPartyDesc(
	void)
{
//	const CHANNELID nMyParty = c_PlayerGetPartyId();

	if (/*nMyParty != INVALID_CHANNELID &&*/ gpUIPartyList)
	{
		for (int i=0; i<gnPartyListCount; ++i)
		{
			if (!PStrCmp(gpUIPartyList[i].m_szPartyName, gApp.m_wszPlayerName) &&
				gpUIPartyList[i].m_nPartyID != INVALID_CHANNELID)
//			if (gpUIPartyList[i].m_nPartyID == (int)nMyParty)
			{
				return &(gpUIPartyList[i]);
			}
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIPartyPanelRemoveMyPartyDesc(
	void)
{
	UI_PARTY_DESC * pParty = sUIPartyPanelGetMyPartyDesc();

	if (pParty && pParty->m_nPartyID != INVALID_CHANNELID)
	{
		pParty->m_nPartyID = INVALID_CHANNELID;
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPartyPanelContextChange(
	UI_COMPONENT *component,
	UI_PARTY_DESC *pParty /*= (UI_PARTY_DESC*)(-1)*/,
	BOOL bInParty /*= (BOOL)(-1)*/,
	BOOL bIsPartyLeader /*= (BOOL)(-1)*/,
	BOOL bPartyIsAdvertised /*= (BOOL)(-1)*/,
	BOOL bAdvertiseSelf /*= (BOOL)(-1)*/)
{
	ASSERT_RETURN(component);

	if (pParty == (UI_PARTY_DESC*)(-1))
	{
		pParty = sUIPartyPanelGetMyPartyDesc();
	}

	if (bInParty == (BOOL)(-1))
	{
		bInParty = (
			(c_PlayerGetPartyId() != INVALID_CHANNELID) ||
			(c_PlayerGetPartyMemberCount() >= 2)
			// || pParty
			);
	}

	if (bIsPartyLeader == (BOOL)(-1))
	{
		bIsPartyLeader =
			(c_PlayerGetPartyLeader() == c_PlayerGetMyGuid());
	}

	if (bPartyIsAdvertised == (BOOL)(-1))
	{
		bPartyIsAdvertised =
			(pParty != NULL);
	}

	if (bAdvertiseSelf == (BOOL)(-1))
	{
		bAdvertiseSelf =
			(pParty && pParty->m_bForPlayerAdvertisementOnly);
	}

	//set the "add to list button" appropriately based on party status
	UI_COMPONENT *pListButton = UIComponentFindChildByName(component, "create party button");
	if (pListButton)
	{
		UI_COMPONENT *pListButtonLabel = UIComponentFindChildByName(pListButton, "create party button label");
		if (pListButtonLabel)
		{
			UI_LABELEX *pListLabel = UICastToLabel(pListButtonLabel);

			UIPartyCreateEnableEdits(!bPartyIsAdvertised, c_PlayerGetPartyMemberCount() < 2);

			LPCWSTR szTooltipString = NULL;

			if (bInParty)
			{
				// in a party - "list my party"
				UILabelSetText(pListLabel, StringTableGetStringByKey("ui partypanel list my party"));

				// are we the party leader?
				if (bIsPartyLeader || bAdvertiseSelf)
				{
					szTooltipString = StringTableGetStringByKey("ui partypanel list my party tooltip");
					UIComponentSetActive(pListButton, TRUE);
				}
				else
				{
					szTooltipString = StringTableGetStringByKey("ui partypanel list my party tooltip2");
					UIComponentSetActive(pListButton, FALSE);
				}

				// fill in appropriate values
				LPWSTR szPartyDesc = NULL;
				int nMaxPlayers = 0;
				int nMinLevel = 0;
				int nMaxLevel = 0;
				BOOL bForPlayerAdvertisementOnly = (BOOL)(-1);

				if (pParty)
				{
					szPartyDesc = pParty->m_szPartyDesc;
					nMaxPlayers = pParty->m_nMaxPlayers;
					nMinLevel = pParty->m_nMinLevel;
					nMaxLevel = pParty->m_nMaxLevel;
					bForPlayerAdvertisementOnly = pParty->m_bForPlayerAdvertisementOnly;
				}
				else
				{
					CHAT_PARTY_INFO partyInfo;
					if (c_PlayerGetPartyInfo(partyInfo))
					{
						szPartyDesc = partyInfo.wszPartyDesc;
						nMaxPlayers = partyInfo.wMaxMembers;
						nMinLevel = partyInfo.wMinLevel;
						nMaxLevel = partyInfo.wMaxLevel;
						bForPlayerAdvertisementOnly = partyInfo.bForPlayerAdvertisementOnly;
						UIPartyCreateEnableEdits(!partyInfo.bIsAdvertised, c_PlayerGetPartyMemberCount() < 2);
					}
				}

				UI_COMPONENT *pComp;
				WCHAR wszText[256];
				
				pComp = UIComponentFindChildByName(component, "party desc edit");
				if (pComp && szPartyDesc)
				{
					UILabelSetText(UICastToLabel(pComp), szPartyDesc);
				}

				pComp = UIComponentFindChildByName(component, "max players edit");
				if (pComp && nMaxPlayers)
				{
					PIntToStr(wszText, arrsize(wszText), nMaxPlayers);
					UILabelSetText(UICastToLabel(pComp), wszText);
				}

				pComp = UIComponentFindChildByName(component, "level min edit");
				if (pComp && nMinLevel)
				{
					PIntToStr(wszText, arrsize(wszText), nMinLevel);
					UILabelSetText(UICastToLabel(pComp), wszText);
				}

				pComp = UIComponentFindChildByName(component, "level max edit");
				if (pComp && nMaxLevel)
				{
					PIntToStr(wszText, arrsize(wszText), nMaxLevel);
					UILabelSetText(UICastToLabel(pComp), wszText);
				}

				pComp = UIComponentFindChildByName(component, "createparty btn");
				if (pComp && bForPlayerAdvertisementOnly != (BOOL)(-1))
				{
					UIButtonSetDown(UICastToButton(pComp), !bForPlayerAdvertisementOnly);
				}
			}
			else
			{
				// not in a party - "list self"
				UILabelSetText(pListLabel, StringTableGetStringByKey("ui partypanel list self"));
				szTooltipString = StringTableGetStringByKey("ui partypanel list self tootlip");
				UIComponentSetActive(pListButton, TRUE);
			}

			if (szTooltipString)
			{
				if( !pListButton->m_szTooltipText )
				{
					pListButton->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
				}
				PStrCopy(pListButton->m_szTooltipText, szTooltipString, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListPanelOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (msg == UIMSG_POSTACTIVATE)
	{
		{
			CEVENT_DATA fakedata;
			sRefreshPartyAdList(NULL,fakedata,0);
			sRefreshPartyMembersList(NULL,fakedata,0);
		}

		//clear the party create result message
		UI_COMPONENT *pResComp = UIComponentFindChildByName(component, "party create result label");
		if (pResComp)
		{
			UI_LABELEX *pResultLabel = UICastToLabel(pResComp);
			UILabelSetText(pResultLabel, L"");
		}

		UIPartyPanelContextChange(component);

		UI_BUTTONEX *pShowPartiesButton = UICastToButton(UIComponentFindChildByName(component, "show parties btn"));
		UI_BUTTONEX *pShowIndividualsButton = UICastToButton(UIComponentFindChildByName(component, "show individuals btn"));

		if (pShowPartiesButton && !UIButtonGetDown(pShowPartiesButton) &&
			pShowIndividualsButton && !UIButtonGetDown(pShowIndividualsButton))
		{
			UIButtonSetDown(pShowPartiesButton, TRUE);
			UIButtonSetDown(pShowIndividualsButton, TRUE);
			UIComponentHandleUIMessage(pShowPartiesButton, UIMSG_PAINT, 0, 0);
			UIComponentHandleUIMessage(pShowIndividualsButton, UIMSG_PAINT, 0, 0);
		}

		if( AppIsHellgate() )
		{
			InputResetMousePosition();		// center the cursor on the screen
		}

		UI_COMPONENT *pPartyMembers = UIComponentFindChildByName(component, "partymembers panel");
		if (pPartyMembers)
		{
			UIComponentHandleUIMessage(pPartyMembers, UIMSG_PAINT, 0, 0);
		}
	}
	else if (msg == UIMSG_POSTINACTIVATE)
	{
		CSchedulerCancelEvent(sgdwRefreshingPartyAdTicket);
		sgdwRefreshingPartyAdTicket = INVALID_ID;

		CSchedulerCancelEvent(sgdwRefreshingPartyMembersTicket);
		sgdwRefreshingPartyMembersTicket = INVALID_ID;

		//UIPartyListClose();
	}

	UNIT * pPlayer = UIGetControlUnit();
	if (pPlayer)
	{
		static const int DEFAULT_PARTY_MEMBERS = 5;
		static const int LEVEL_PLUS_MINUS = 3;

		const int nPlayerLevel = UnitGetStat( pPlayer, STATS_LEVEL );
		const int nLevelRangeMin = max( 1, nPlayerLevel - LEVEL_PLUS_MINUS );
		const int nLevelRangeMax = min( 50, nPlayerLevel + LEVEL_PLUS_MINUS );  //I can't believe we don't have a const for this someplace.
		WCHAR wszText[5];
		UI_COMPONENT * pComp;

		pComp = UIComponentFindChildByName(component->m_pParent, "max players edit");
		if (pComp)
		{
			UI_LABELEX *pPartyMaxMembersLbl = UICastToLabel(pComp);
			if (!pPartyMaxMembersLbl->m_Line.HasText())
			{
				PIntToStr(wszText, arrsize(wszText), DEFAULT_PARTY_MEMBERS);
				UILabelSetText(pPartyMaxMembersLbl, wszText);
			}
		}

		pComp = UIComponentFindChildByName(component->m_pParent, "level min edit");
		if (pComp)
		{
			UI_LABELEX *pPartyMinLevelLbl = UICastToLabel(pComp);
			if (!pPartyMinLevelLbl->m_Line.HasText())
			{
				PIntToStr(wszText, arrsize(wszText), nLevelRangeMin);
				UILabelSetText(pPartyMinLevelLbl, wszText);
			}
		}

		pComp = UIComponentFindChildByName(component->m_pParent, "level max edit");
		if (pComp)
		{
			UI_LABELEX *pPartyMaxLevelLbl = UICastToLabel(pComp);
			if (!pPartyMaxLevelLbl->m_Line.HasText())
			{
				PIntToStr(wszText, arrsize(wszText), nLevelRangeMax);
				UILabelSetText(pPartyMaxLevelLbl, wszText);
			}
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPartyRequestMembers(int nPartyID)
{
	UI_PARTY_DESC *tParty = sGetCurrentPartyDesc();
	if (tParty)
	{
		tParty->m_nPlayersRecieved = 0;
	}

	CHAT_REQUEST_MSG_GET_PARTY_MEMBERS getMbrsMsg;
	getMbrsMsg.TagPartyChannel = nPartyID;
	c_ChatNetSendMessage(
		&getMbrsMsg,
		USER_REQUEST_GET_PARTY_MEMBERS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}


	// get mouse relative to the component
	x -= pos.m_fX;
	y -= pos.m_fY;
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);
	// compensate for scrolling
	x += UIComponentGetScrollPos(component).m_fX;
	y += UIComponentGetScrollPos(component).m_fY;


	UI_COMPONENT* pHilightBar = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_PARTYLIST_HILIGHT) : UIComponentFindChildByName(component->m_pParent, "party list highlight");
	ASSERT_RETVAL(pHilightBar, UIMSG_RET_NOT_HANDLED);

	UI_RECT rectElement;
	UI_PARTY_DESC *pParty = gpUIPartyList;
	int i = 0;
	while (pParty && i < gnPartyListCount)
	{
		//	don't check removed entries...
		if( gpUIPartyList[i].m_nPartyID == -1 ) {
			pParty++;
			i++;
			continue;
		}

		if (UIElementCheckBounds(pParty->m_pNameElement, x, y) ||
			UIElementCheckBounds(pParty->m_pDescElement, x, y) )
		{

			component->m_dwData = (DWORD)pParty->m_nPartyID;
			UIPartyRequestMembers(pParty->m_nPartyID);

			if (pHilightBar)
			{
				pHilightBar->m_Position.m_fY = pParty->m_pNameElement->m_Position.m_fY;
				UIComponentSetVisible(pHilightBar, TRUE);
			}

			UI_COMPONENT *pPartyPanel = UIComponentGetByEnum(UICOMP_PARTYPANEL);
			if (pPartyPanel)
			{
				UI_COMPONENT *pButton = NULL;
				const BOOL bIsMyParty = ((CHANNELID)pParty->m_nPartyID == c_PlayerGetPartyId());

				pButton = UIComponentFindChildByName(pPartyPanel, "invite button");
				if (pButton)
				{
					UIComponentSetVisible(pButton, TRUE);
					UIComponentSetActive(pButton, pParty->m_bForPlayerAdvertisementOnly && pParty->m_nCurrentPlayers < 2 && (c_PlayerGetPartyId() == INVALID_CHANNELID || c_PlayerGetPartyLeader() == c_PlayerGetMyGuid()) && !bIsMyParty);
				}

				pButton = UIComponentFindChildByName(pPartyPanel, "join button");
				if (pButton)
				{
					UIComponentSetVisible(pButton, TRUE);
					UIComponentSetActive(pButton, !pParty->m_bForPlayerAdvertisementOnly && !bIsMyParty);
				}

				pButton = UIComponentFindChildByName(pPartyPanel, "whisper button");
				if (pButton)
				{
					UIComponentSetVisible(pButton, TRUE);
					UIComponentSetActive(pButton, !bIsMyParty);
				}
			}


			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		pParty++;
		i++;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// get mouse relative to the component
	x -= pos.m_fX;
	y -= pos.m_fY;
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);

	// compensate for scrolling
	x += UIComponentGetScrollPos(component).m_fX;
	y += UIComponentGetScrollPos(component).m_fY;

	UI_RECT rectElement;
	UI_PARTY_DESC *pParty = gpUIPartyList;
	int i = 0;
	while (pParty && i < gnPartyListCount)
	{
		//	don't check removed entries...
		if( gpUIPartyList[i].m_nPartyID == -1 ||
			!pParty->m_pNameElement ||
			!pParty->m_pDescElement) {
			pParty++;
			i++;
			continue;
		}

		DWORD dwColor = pParty->m_bForPlayerAdvertisementOnly ? GFXCOLOR_WHITE : GetFontColor(FONTCOLOR_FSDARKORANGE);
		if (component->m_dwData == (DWORD)gpUIPartyList[i].m_nPartyID)
		{
			dwColor = GetFontColor(FONTCOLOR_FSORANGE);
		}
		else if (UIElementCheckBounds(pParty->m_pNameElement, x, y) ||
			UIElementCheckBounds(pParty->m_pDescElement, x, y) )
		{
			dwColor = GFXCOLOR_YELLOW;
		}

		pParty->m_pNameElement->m_dwColor =
		pParty->m_pDescElement->m_dwColor = dwColor;

		pParty++;
		i++;
	}

	UISetNeedToRender(component);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
#define UI_PARTY_MEMBERS 5
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMembersOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);

	UI_PARTY_DESC *pCurrentParty = sGetCurrentPartyDesc();
	if (pCurrentParty)
	{
		WCHAR szTemp[32];
		UIComponentAddFrame(component);

		UI_COMPONENT *pChild = UIComponentFindChildByName(component, "party name" );	
		if (pChild)
		{
			UILabelSetText(pChild, pCurrentParty->m_szPartyName);
		}
		pChild = UIComponentFindChildByName(component, "party desc" );	
		if (pChild)
		{
			UILabelSetText(pChild, pCurrentParty->m_szPartyDesc);
		}
		pChild = UIComponentFindChildByName(component, "party size" );	
		if (pChild)
		{
			PStrPrintf(szTemp, 32, L"%d / %d", pCurrentParty->m_nCurrentPlayers, pCurrentParty->m_nMaxPlayers);
			UILabelSetText(pChild, szTemp);
		}
		pChild = UIComponentFindChildByName(component, "party levels" );
		if (pChild)
		{
			PStrPrintf(szTemp, 32, L"%d - %d", pCurrentParty->m_nMinLevel, pCurrentParty->m_nMaxLevel);
			UILabelSetText(pChild, szTemp);
		}

		UI_COMPONENT *pPanel = UIComponentFindChildByName(component, "members" );
		if( !pPanel )
			return UIMSG_RET_NOT_HANDLED;
		pChild = pPanel->m_pFirstChild;

		// names
		for (int i=0; i< UI_PARTY_MEMBERS; i++)
		{
			if( !pChild )
				return UIMSG_RET_NOT_HANDLED;
			if (i >= pCurrentParty->m_nPlayersRecieved)
			{
				UILabelSetText(pChild, L"");
			}
			else
			{
				UILabelSetText(pChild, pCurrentParty->m_tPartyMembers[i].m_szName);
			}
			pChild = pChild->m_pNextSibling;
		}

		// classes
		for (int i=0; i< UI_PARTY_MEMBERS; i++)
		{
			if( !pChild )
				return UIMSG_RET_NOT_HANDLED;
			if (i >= pCurrentParty->m_nPlayersRecieved)
			{
				UILabelSetText(pChild, L"");
			}
			else
			{
				int nCharClass = pCurrentParty->m_tPartyMembers[i].m_nPlayerUnitClass;

				const UNIT_DATA * pPlayerData = PlayerGetData(AppGetCltGame(), nCharClass);
				if (pPlayerData)
				{
					UILabelSetTextByStringIndex(pChild, pPlayerData->nString);
				}
				else
				{
					UILabelSetText( pChild, L"?" );
				}
			}
			pChild = pChild->m_pNextSibling;
		}

		// levels
		for (int i=0; i< UI_PARTY_MEMBERS; i++)
		{
			if( !pChild )
				return UIMSG_RET_NOT_HANDLED;
			if (i >= pCurrentParty->m_nPlayersRecieved)
			{
				UILabelSetText(pChild, L"");
			}
			else if (pCurrentParty->m_tPartyMembers[i].m_nRank > 0)
			{
				// show rank
				WCHAR szLevel[256], szRank[256], puszText[256];
				LanguageFormatIntString(szLevel, arrsize(szRank), pCurrentParty->m_tPartyMembers[i].m_nLevel);
				LanguageFormatIntString(szRank, arrsize(szRank), pCurrentParty->m_tPartyMembers[i].m_nRank);
				PStrCopy(puszText, GlobalStringGet(GS_LEVEL_WITH_RANK), arrsize(puszText));
				PStrReplaceToken( puszText, arrsize(puszText), StringReplacementTokensGet( SR_LEVEL ), szLevel );
				PStrReplaceToken( puszText, arrsize(puszText), StringReplacementTokensGet( SR_RANK ), szRank );
				UILabelSetText(pChild, puszText);
			}
			else
			{
				UILabelSetText(pChild, PIntToStr(szTemp, 32, pCurrentParty->m_tPartyMembers[i].m_nLevel));
			}
			pChild = pChild->m_pNextSibling;
		}

		UIComponentSetActive(component, TRUE);
		return UIMSG_RET_HANDLED;
	}

	UIComponentSetVisible(component, FALSE);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPartyCreateEnableEdits(
	BOOL bEnable,
	BOOL bPartyCreateButton /*= -1*/)
{
	UI_COMPONENT *pPartyPanel = UIComponentGetByEnum(UICOMP_PARTYPANEL);
	if (pPartyPanel)
	{
		UI_COMPONENT *pEdit = NULL;
		while ((pEdit = UIComponentIterateChildren(pPartyPanel, pEdit, UITYPE_EDITCTRL, TRUE)) != NULL)
		{
			UIComponentSetActive(pEdit, bEnable);
			UI_LABELEX *pEditLabel = UICastToLabel(pEdit);
			pEditLabel->m_bDisabled = !bEnable;
			UIComponentHandleUIMessage(pEdit, UIMSG_PAINT, 0, 0);
		}

		pEdit = UIComponentFindChildByName(pPartyPanel, "createparty btn");
		if (pEdit)
		{
			if (bPartyCreateButton == -1)
			{
				bPartyCreateButton = UIComponentGetVisible(pEdit);
			}
			UIComponentSetVisible(pEdit, TRUE);
			UIComponentSetActive(pEdit, bEnable);
			UIComponentSetVisible(pEdit, bPartyCreateButton);
			UIComponentHandleUIMessage(pEdit, UIMSG_PAINT, 0, 0);
		}

		UI_COMPONENT *pCreateButton = UIComponentFindChildByName(pPartyPanel, "create button");
		if (pCreateButton)
		{
			UI_COMPONENT *pCreateButtonLabel = UIComponentIterateChildren(pCreateButton, NULL, UITYPE_LABEL, FALSE);
			if (pCreateButtonLabel)
			{
				if (bEnable)
				{
					UILabelSetText(pCreateButtonLabel, StringTableGetStringByKey("ui partypanel ok button"));
				}
				else
				{
					UILabelSetText(pCreateButtonLabel, StringTableGetStringByKey("ui partypanel unlist button"));
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyCreateBtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pCreateButtonLabel = UIComponentIterateChildren(component, NULL, UITYPE_LABEL, FALSE);
	if (pCreateButtonLabel  && 
		c_PlayerGetPartyId() != INVALID_CHANNELID)
	{
		// user pressed "unlist"
		UILabelSetText(pCreateButtonLabel, StringTableGetStringByKey("ui partypanel ok button"));
		UIPartyCreateEnableEdits(TRUE);

		// unadvertise if there are actual members in our party
		if( c_PlayerGetPartyMemberCount() > 1 )
		{
			MSG_CCMD_PARTY_UNADVERTISE unlistMsg;
			c_SendMessage(CCMD_PARTY_UNADVERTISE, &unlistMsg);
		}
		else // otherwise, kill it! Otherwise we have an invisible party.
		{
			CHAT_REQUEST_MSG_DISBAND_PARTY disbandMsg;
			c_ChatNetSendMessage(&disbandMsg, USER_REQUEST_DISBAND_PARTY);
		}
		
		UIPartyPanelRemoveMyPartyDesc();

		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	UI_COMPONENT *pParent = component->m_pParent;
	if( AppIsTugboat() )
	{
		pParent = pParent->m_pParent;
	}

	//UI_COMPONENT *pComp = UIComponentFindChildByName(pParent, "party name edit");
	//if (!pComp)
	//	return UIMSG_RET_NOT_HANDLED;
	//UI_LABELEX *pPartyNameLbl = UICastToLabel(pComp);

	//if (!pPartyNameLbl->m_Line.m_szText || !pPartyNameLbl->m_Line.m_szText[0])
	//{
	//	const WCHAR *puszTitle = GlobalStringGet( GS_ENTER_PARTY_NAME_TITLE );
	//	const WCHAR *puszText = GlobalStringGet( GS_ENTER_PARTY_NAME );
	//	
	//	UIShowGenericDialog( puszTitle, puszText );
	//	return UIMSG_RET_HANDLED_END_PROCESS;
	//}

	UI_COMPONENT *pComp = UIComponentFindChildByName(pParent, "party desc edit");
	if (!pComp)
		return UIMSG_RET_NOT_HANDLED;
	UI_LABELEX *pPartyDescLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pParent, "max players edit");
	if (!pComp)
		return UIMSG_RET_NOT_HANDLED;
	UI_LABELEX *pPartyMaxMembersLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pParent, "level min edit");
	if (!pComp)
		return UIMSG_RET_NOT_HANDLED;
	UI_LABELEX *pPartyMinLevelLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pParent, "level max edit");
	if (!pComp)
		return UIMSG_RET_NOT_HANDLED;
	UI_LABELEX *pPartyMaxLevelLbl = UICastToLabel(pComp);

	int nMaxMembers = ((pPartyMaxMembersLbl->m_Line.HasText())? PStrToInt(pPartyMaxMembersLbl->m_Line.GetText()) : 0);
	int nMinLevel = ((pPartyMinLevelLbl->m_Line.HasText())? PStrToInt(pPartyMinLevelLbl->m_Line.GetText()) : 0);
	int nMaxLevel = ((pPartyMaxLevelLbl->m_Line.HasText())? PStrToInt(pPartyMaxLevelLbl->m_Line.GetText()) : 0);

	//	TODO: clear out fields upon success???

	BOOL bLFG = FALSE;
	UI_COMPONENT *pPartyPanel = AppIsHellgate() ? UIComponentGetByEnum(UICOMP_PARTYPANEL) : pParent;
	if (pPartyPanel)
	{
		UI_COMPONENT *pCompButton = UIComponentFindChildByName(pPartyPanel, "createparty btn");
		if (pCompButton)
		{
			UI_BUTTONEX *pButton = UICastToButton( pCompButton );
			if (pButton && UIComponentGetActive( pButton ) && !UIButtonGetDown( pButton ))			
			{
				c_SendPlayerAdvertiseLFG(pPartyDescLbl->m_Line.GetText(), nMaxMembers, nMinLevel, nMaxLevel );				
				bLFG = TRUE;
			}
		}
	}

	if (!bLFG)
	{
		c_SendPartyAdvertise(pPartyDescLbl->m_Line.GetText(), nMaxMembers, nMinLevel, nMaxLevel );	
	}

	UIPartyCreateEnableEdits(FALSE);
	UIComponentSetActive(component, FALSE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListHeaderPartyNameOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	sUIPartyListSort(0);
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_PARTYLIST), UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListHeaderPartyNumberOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	sUIPartyListSort(1);
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_PARTYLIST), UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyListHeaderPartyRangeOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	sUIPartyListSort(2);
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_PARTYLIST), UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPartyCreateOnServerResult(
	UI_PARTY_CREATE_RESULT eResult)
{
	UI_COMPONENT *pKiosk = UIComponentGetByEnum(UICOMP_PARTYPANEL);

	BOOL bSuccess = FALSE;

	const WCHAR *szResultString = NULL;
	switch (eResult)
	{
		case PCR_SUCCESS:
			{
				//{
				//	UI_COMPONENT *pComp = pResComp->m_pParent->m_pFirstChild;
				//	while (pComp)
				//	{
				//		if (pComp->m_eComponentType == UITYPE_EDITCTRL)
				//		{
				//			UI_EDITCTRL *pEdit = UICastToEditCtrl(pComp);
				//			{
				//				UILabelSetText(pEdit, L"");
				//			}
				//		}
				//		pComp = pComp->m_pNextSibling;
				//	}
				//}
//				szResultString = GlobalStringGet( GS_UI_PARTY_CREATE_SUCCESS );
//				if (pKiosk)
//				{
//					UI_PANELEX *pPanel = UICastToPanel(pKiosk);
//					UIPanelSetTab(pPanel, 0);
//				}

				CEVENT_DATA fakedata;
				sRefreshPartyAdList(NULL,fakedata,0);

				bSuccess = TRUE;
			}
			break;
		case PCR_INVALID_PARTY_NAME:
			szResultString = GlobalStringGet( GS_UI_PARTY_CREATE_INVALID_NAME );
			break;
		case PCR_INVALID_PARTY_LEVEL_RANGE:
			szResultString = GlobalStringGet( GS_UI_PARTY_CREATE_INVALID_LEVEL_RANGE );
			break;
		case PCR_ALREADY_IN_A_PARTY:
			szResultString = GlobalStringGet( GS_UI_PARTY_CREATE_INPARTY );
			break;
		case PCR_PARTY_NAME_TAKEN:
			szResultString = GlobalStringGet( GS_UI_PARTY_CREATE_NAME_ALREADY_TAKEN );
			break;
		case PCR_ERROR:
		default:
			szResultString = GlobalStringGet( GS_UI_PARTY_CREATE_FAILURE );
			break;
	};

	if (szResultString)
	{
		UIShowGenericDialog(StringTableGetStringByKey("ui create party button"), szResultString);
	}

//	UIPartyPanelContextChange(pKiosk);

	UI_COMPONENT *pCreateButton = UIComponentFindChildByName(pKiosk, "create button");
	if (pCreateButton)
	{
		UIComponentSetActive(pCreateButton, TRUE);
	//	UI_COMPONENT *pCreateButtonLabel = UIComponentIterateChildren(pCreateButton, NULL, UITYPE_LABEL, FALSE);
	//	if (pCreateButtonLabel)
	//	{
	//		if (bSuccess)
	//		{
	//			UILabelSetText(pCreateButtonLabel, StringTableGetStringByKey("ui partypanel unlist button"));
	//		}
	//		else
	//		{
	//			UILabelSetText(pCreateButtonLabel, StringTableGetStringByKey("ui create party create button"));
	//			UIPartyCreateEnableEdits(TRUE);
	//		}
	//	}
	}

	// Refresh player match panel and buttons
	if (UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL))
	{
		UIPlayerMatchDialogContextChange();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyInviteBtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();
	if (pParty && pParty->m_szPartyName)
	{
		c_PlayerInvite(pParty->m_szPartyName);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyJoinBtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();

	if (pParty)
	{
		CHAT_REQUEST_MSG_JOIN_PARTY joinMsg;
		joinMsg.TagChannel = pParty->m_nPartyID;
		c_ChatNetSendMessage(
			&joinMsg,
			USER_REQUEST_JOIN_PARTY );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyWhisperBtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();
	if (pParty && pParty->m_szPartyName)
	{
		c_PlayerSetupWhisperTo(pParty->m_szPartyName);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}




















//----------------------------------------------------------------------------
// KCK: New Player Matching dialog stuff starts here
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// These are our scheduled refresh events to keep our various list current
//----------------------------------------------------------------------------

#define PLAYER_MATCH_LIST_REFRESH_RATE 10

static DWORD sgdwRefreshingPartyPlayerListTicket = INVALID_ID;
static DWORD sgdwRefreshingMatchListTicket = INVALID_ID;

// This refreshes both the party and player lists, as they use the same data with a filter.
static void sRefreshPartyPlayerList(
	GAME*,
	const CEVENT_DATA&,
	DWORD)
{
	if (sgdwRefreshingPartyPlayerListTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(sgdwRefreshingPartyPlayerListTicket);
	}

	//	TODO: parties can be classified by string search patterns, should be used eventually...
	CHAT_REQUEST_MSG_GET_ADVERTISED_PARTIES getPartiesMsg;
	c_ChatNetSendMessage(
		&getPartiesMsg,
		USER_REQUEST_GET_ADVERTISED_PARTIES );

	TIME time = CSchedulerGetTime() + (PLAYER_MATCH_LIST_REFRESH_RATE * SCHEDULER_TIME_PER_SECOND);
	CEVENT_DATA nodata;
	sgdwRefreshingPartyPlayerListTicket = CSchedulerRegisterEvent(
		time,
		sRefreshPartyPlayerList,
		nodata);
}

// This refreshes the match list
static void sRefreshMatchList(
	GAME*,
	const CEVENT_DATA&,
	DWORD)
{
	if (sgdwRefreshingMatchListTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(sgdwRefreshingMatchListTicket);
	}

	MSG_CCMD_LIST_GAME_INSTANCES msg;
	msg.nTypeOfInstance = GAME_INSTANCE_PVP;
	c_SendMessage(CCMD_LIST_GAME_INSTANCES, &msg);

	TIME time = CSchedulerGetTime() + (PLAYER_MATCH_LIST_REFRESH_RATE * SCHEDULER_TIME_PER_SECOND);
	CEVENT_DATA nodata;
	sgdwRefreshingMatchListTicket = CSchedulerRegisterEvent(
		time,
		sRefreshMatchList,
		nodata);
}

//----------------------------------------------------------------------------
// Set the current context of the player matching dialog and update buttons
//----------------------------------------------------------------------------
void UIPlayerMatchDialogContextChange(PlayerMatchModes nMode)
{
	UI_COMPONENT *pPlayerMatchDialog = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	UI_PANELEX * pPanel = UICastToPanel(pPlayerMatchDialog);
	ASSERTV_RETURN(pPanel, "UIPlayerMatchDialogContextChange: pPanel is NULL. Contact Kevin Klemmick");

	BOOL	bFirstTimeIn = (s_CurrentPlayerMatchMode == nMode)? FALSE : TRUE;

	if (s_CurrentPlayerMatchMode < PMM_PARTY_CREATE)
		s_LastTab = s_CurrentPlayerMatchMode;
	
	UIPanelSetTab(pPanel, nMode);
	s_CurrentPlayerMatchMode = nMode;

	UI_PANELEX * pCurTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
	ASSERTV_RETURN(pCurTab, "UIPlayerMatchDialogContextChange: pCurTab is NULL. Contact Kevin Klemmick");

	UI_PARTY_DESC * pParty = sUIPartyPanelGetMyPartyDesc();
	UI_COMPONENT * pHilightBar = UIComponentFindChildByName(pCurTab, "list highlight");

	BOOL bInParty = ( (c_PlayerGetPartyId() != INVALID_CHANNELID) ||
					  (c_PlayerGetPartyMemberCount() >= 2) );
	BOOL bSelected = (pHilightBar && UIComponentGetVisible(pHilightBar));

	const WCHAR * szTooltipString = NULL;

	// Set the action buttons based on our context
	UI_COMPONENT *pButton1 = UIComponentFindChildByName(pPlayerMatchDialog, "action1 button");
	UI_COMPONENT *pButton2 = UIComponentFindChildByName(pPlayerMatchDialog, "action2 button");
	UI_COMPONENT *pButton3 = UIComponentFindChildByName(pPlayerMatchDialog, "action3 button");
	if (pButton1)
	{
		if (!pButton1->m_szTooltipText)
			pButton1->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
		UI_COMPONENT* child = pButton1->m_pFirstChild;
		if (UIComponentIsLabel(child))
		{
			UI_LABELEX *pLabel = UICastToLabel(child);
			UIComponentSetActive(pButton1, TRUE);
			if (nMode == PMM_PARTY_CREATE || nMode == PMM_LIST_SELF || 
				nMode == PMM_MATCH_CREATE || nMode == PMM_AUTOMATCH)
			{
				UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel cancel button"));
				szTooltipString = StringTableGetStringByKey("ui partypanel cancel button");
			}
			else
			{
				if (nMode == PMM_PARTY)
				{
					if (!bInParty || (bInParty && pParty && pParty->m_bForPlayerAdvertisementOnly))
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel join button"));
						UIComponentSetActive(pButton1, bSelected);
						szTooltipString = StringTableGetStringByKey("ui partypanel join party tooltip");
					}
					else
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel leave button"));
						szTooltipString = StringTableGetStringByKey("ui partypanel leave party tooltip");
					}
				}
				if (nMode == PMM_PLAYER)
				{
					if (!bInParty || (bInParty && pParty && !pParty->m_bForPlayerAdvertisementOnly))
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel list self"));
						szTooltipString = StringTableGetStringByKey("ui partypanel list self tooltip");
					}
					else
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel unlist button"));
						szTooltipString = StringTableGetStringByKey("ui partypanel unlist tooltip");
					}
				}
				if (nMode == PMM_MATCH)
				{
					UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel join button"));
					UIComponentSetActive(pButton1, bSelected);
					szTooltipString = StringTableGetStringByKey("ui partypanel join match tooltip");
				}
			}
		}
		if (szTooltipString)
			PStrCopy(pButton1->m_szTooltipText, szTooltipString, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
	}
	if (pButton2)
	{
		if (!pButton2->m_szTooltipText)
			pButton2->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
		if (nMode == PMM_PARTY_CREATE || nMode == PMM_LIST_SELF || 
			nMode == PMM_MATCH_CREATE || nMode == PMM_AUTOMATCH)
		{
			UIComponentSetActive(pButton2, FALSE);
			UIComponentSetVisible(pButton2, FALSE);
		}
		else
		{
			UIComponentSetActive(pButton2, TRUE);
			UIComponentSetVisible(pButton2, TRUE);
			UI_COMPONENT* child = pButton2->m_pFirstChild;
			if (UIComponentIsLabel(child))
			{
				UI_LABELEX *pLabel = UICastToLabel(child);
				if (nMode == PMM_PARTY)
				{
					if (!bInParty || (bInParty && pParty && pParty->m_bForPlayerAdvertisementOnly))
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel create button"));
						szTooltipString = StringTableGetStringByKey("ui partypanel create party tooltip");
					}
					else
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel challenge button"));
						UIComponentSetActive(pButton2, bSelected);
						szTooltipString = StringTableGetStringByKey("ui partypanel challenge party tooltip");
					}
				}
				if (nMode == PMM_PLAYER)
				{
					UIComponentSetActive(pButton2, bSelected);
					if (bInParty && pParty && !pParty->m_bForPlayerAdvertisementOnly)
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel invite button"));
						szTooltipString = StringTableGetStringByKey("ui partypanel invite tooltip");
					}
					else
					{
						UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel challenge button"));
						szTooltipString = StringTableGetStringByKey("ui partypanel challenge player tooltip");
					}
				}
				if (nMode == PMM_MATCH)
				{
					UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel create button"));
					szTooltipString = StringTableGetStringByKey("ui partypanel create match tooltip");
				}
			}
		}
		if (szTooltipString)
			PStrCopy(pButton2->m_szTooltipText, szTooltipString, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
	}
	if (pButton3)
	{
		if (!pButton3->m_szTooltipText)
			pButton3->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
		UI_COMPONENT* child = pButton3->m_pFirstChild;
		if (UIComponentIsLabel(child))
		{
			UI_LABELEX *pLabel = UICastToLabel(child);
			UIComponentSetActive(pButton3, TRUE);
			if (nMode == PMM_PARTY_CREATE || nMode == PMM_LIST_SELF || 
				nMode == PMM_MATCH_CREATE || nMode == PMM_AUTOMATCH)
			{
				UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel ok button"));
				szTooltipString = StringTableGetStringByKey("ui partypanel ok button");
			}
			else
			{
				UILabelSetText(pLabel, StringTableGetStringByKey("ui partypanel automatch button"));
				szTooltipString = StringTableGetStringByKey("ui partypanel automatch tooltip");
				const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
				if ( pBadges && pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
				{
					UIComponentSetActive(pButton3, FALSE);
					szTooltipString = StringTableGetStringByKey("mode_not_available_with_trial");
				}
			}
		}
		if (szTooltipString)
			PStrCopy(pButton3->m_szTooltipText, szTooltipString, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
	}

	// Set focus to the description box if this is the first time we've gone into this tab
	if (bFirstTimeIn && (nMode == PMM_PARTY_CREATE || nMode == PMM_LIST_SELF || nMode == PMM_MATCH_CREATE || nMode == PMM_AUTOMATCH))
	{
		UI_COMPONENT * pEdit = UIComponentFindChildByName(pCurTab, "description edit");
		if (pEdit)
			UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)pEdit->m_idComponent);
	}

	// Clear Highlight
	if (bFirstTimeIn)
	{
		UI_COMPONENT * pPartyList = UIComponentGetByEnum(UICOMP_PARTYLIST);
		if (pPartyList)
			pPartyList->m_dwData = (DWORD_PTR)(-1);
	}

	// Turn the actually list on and off for modes that need it or not.
	UI_COMPONENT * pList = UIComponentFindChildByName(pPlayerMatchDialog, "party list clip");
	if (pList)
	{
		UIComponentSetVisible(pList, (nMode < PMM_PARTY_CREATE));
		UIComponentSetActive(pList, (nMode < PMM_PARTY_CREATE));
	}
	UI_COMPONENT * pDetails = UIComponentFindChildByName(pPlayerMatchDialog, "lower party list panel");
	if (pDetails)
	{
		UIComponentSetVisible(pDetails, (nMode < PMM_PARTY_CREATE));
		UIComponentSetActive(pDetails, (nMode < PMM_PARTY_CREATE));
	}

	// Build the match type listbox contents if we've not already done so.
	UI_COMPONENT *pGametype = UIComponentFindChildByName(pCurTab, "gametype_combo");
	if (pGametype)
	{
		UI_COMBOBOX * pCombo = UICastToComboBox(pGametype);
		if (pCombo && pCombo->m_pListBox->m_LinesList.Count() == 0)
		{
			UITextBoxClear(pCombo->m_pListBox);

			// KCK NOTE: Would be nice to have a list of gametype somewhere. Right now, I'll use GAME_SUBTYPE, but that's not ideal
			UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("ui gametype ctf"), (QWORD)GAME_SUBTYPE_PVP_CTF);
			UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("ui gametype dm"), (QWORD)GAME_SUBTYPE_PVP_PRIVATE);
			UIListBoxSetSelectedIndex(pCombo->m_pListBox, 0);
		}
	}		

	// Build the player listbox contents if we've not already done so.
	UI_COMPONENT *pPlayersPerSide = UIComponentFindChildByName(pCurTab, "players_combo");
	if (pPlayersPerSide)
	{
		UI_COMBOBOX * pCombo = UICastToComboBox(pPlayersPerSide);
		if (pCombo && pCombo->m_pListBox->m_LinesList.Count() == 0)
		{
			UITextBoxClear(pCombo->m_pListBox);

			// KCK NOTE: Would be nice to pull this from somewhere rather than hardcode it..
			UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("ui playermatch 1v1"), 1);
			UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("ui playermatch 5v5"), 5);
			UIListBoxSetSelectedIndex(pCombo->m_pListBox, 0);
		}
	}	

	// Fill in default values for edit boxes
	UI_COMPONENT * pComp = UIComponentFindChildByName(pCurTab, "max players edit");
	if (pComp)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComp);
		if (pLabel && !pLabel->m_Line.HasText())
		{
			// KCK: Yeah, ugly.. Hard coded Max players default
			UILabelSetText(pLabel, L"5");				
		}
	}
	pComp = UIComponentFindChildByName(pCurTab, "level min edit");
	if (pComp)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComp);
		if (pLabel && !pLabel->m_Line.HasText())
		{
			// KCK: Yeah, ugly.. Hard coded min level default
			UILabelSetText(pLabel, L"1");				
		}
	}
	pComp = UIComponentFindChildByName(pCurTab, "level max edit");
	if (pComp)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComp);
		if (pLabel && !pLabel->m_Line.HasText())
		{
			// KCK: Yeah, ugly.. Hard coded Max level default
			UILabelSetText(pLabel, L"50");				
		}
	}

	// Repaint
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_PARTYLIST), UIMSG_PAINT, 0, 0);
	UIComponentHandleUIMessage(UIComponentFindChildByName(pPlayerMatchDialog, "partymembers panel"), UIMSG_PAINT, 0, 0);
}

void UIPlayerMatchDialogContextChange(void)
{
	UIPlayerMatchDialogContextChange(s_CurrentPlayerMatchMode);
}



//----------------------------------------------------------------------------
// Retrieves selected match, if any
//----------------------------------------------------------------------------
static UI_PARTY_DESC * sGetCurrentMatchDesc()
{
	UI_COMPONENT *pPartyList = UIComponentGetByEnum(UICOMP_PARTYLIST);
	if (pPartyList)
	{
		if ((int)pPartyList->m_dwData >= 0 && gpUIMatchList)
		{
			for (int i=0; i<gnMatchListCount; ++i)
			{
				if (gpUIMatchList[i].m_nPartyID == (int)pPartyList->m_dwData)
				{
					return &(gpUIMatchList[i]);
				}
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
// Changes the Player Matching panel to Party mode
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchSetPartyMode(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIPlayerMatchDialogContextChange(PMM_PARTY);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
// Changes the Player Matching to Player mode
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchSetPlayerMode(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIPlayerMatchDialogContextChange(PMM_PLAYER);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
// Changes the Player Matching to Match  mode
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchSetMatchMode(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIPlayerMatchDialogContextChange(PMM_MATCH);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
// Called after Activate and a de-activate
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
 	if (msg == UIMSG_POSTACTIVATE)
	{
		CEVENT_DATA fakedata;
//		sRefreshPartyAdList(NULL,fakedata,0);
//		sRefreshPartyMembersList(NULL,fakedata,0);
		sRefreshPartyPlayerList(NULL, fakedata, 0);
		sRefreshMatchList(NULL, fakedata, 0);

		UIPlayerMatchDialogContextChange(s_LastTab);
	}
	else if (msg == UIMSG_POSTINACTIVATE)
	{
		CSchedulerCancelEvent(sgdwRefreshingPartyPlayerListTicket);
		sgdwRefreshingPartyPlayerListTicket = INVALID_ID;

		CSchedulerCancelEvent(sgdwRefreshingMatchListTicket);
		sgdwRefreshingMatchListTicket = INVALID_ID;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Guild button toggle
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGuildOnlyButtonClicked(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	s_bGuildOnly = !s_bGuildOnly;

	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, s_bGuildOnly);
	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
// All Action Button methods should go here
//----------------------------------------------------------------------------

void UIDoLeaveParty(void)
{
	// unadvertise if there are actual members in our party
	if( c_PlayerGetPartyMemberCount() > 1 )
	{
		CHAT_REQUEST_MSG_LEAVE_PARTY joinMsg;
		c_ChatNetSendMessage(&joinMsg, USER_REQUEST_LEAVE_PARTY );
	}
	else // otherwise, kill it! Otherwise we have an invisible party.
	{
		CHAT_REQUEST_MSG_DISBAND_PARTY disbandMsg;
		c_ChatNetSendMessage(&disbandMsg, USER_REQUEST_DISBAND_PARTY);
	}

	UIPartyPanelRemoveMyPartyDesc();
}

void UIDoCreateParty(void)
{
	UIDoLeaveParty();

	UI_COMPONENT *pPlayerMatchDialog = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	if (!pPlayerMatchDialog || !UIComponentGetVisible(pPlayerMatchDialog))
		return;

	UI_PANELEX * pPanel = UICastToPanel(pPlayerMatchDialog);
	UI_PANELEX * pCurTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
	ASSERTV_RETURN(pCurTab, "UIDoListSelf: pCurTab is NULL. Contact Kevin Klemmick");

	UI_COMPONENT *pComp = UIComponentFindChildByName(pCurTab, "description edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyDescLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pPlayerMatchDialog, "max players edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyMaxMembersLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pPlayerMatchDialog, "level min edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyMinLevelLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pPlayerMatchDialog, "level max edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyMaxLevelLbl = UICastToLabel(pComp);

	int nMaxMembers = ((pPartyMaxMembersLbl->m_Line.HasText())? PStrToInt(pPartyMaxMembersLbl->m_Line.GetText()) : 0);
	int nMinLevel = ((pPartyMinLevelLbl->m_Line.HasText())? PStrToInt(pPartyMinLevelLbl->m_Line.GetText()) : 0);
	int nMaxLevel = ((pPartyMaxLevelLbl->m_Line.HasText())? PStrToInt(pPartyMaxLevelLbl->m_Line.GetText()) : 0);

	c_SendPartyAdvertise(pPartyDescLbl->m_Line.GetText(), nMaxMembers, nMinLevel, nMaxLevel );		

	UIPlayerMatchDialogContextChange(PMM_PARTY);
}

void UIDoListSelf(void)
{
	UIDoLeaveParty();

	UI_COMPONENT *pPlayerMatchDialog = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	if (!pPlayerMatchDialog || !UIComponentGetVisible(pPlayerMatchDialog))
		return;

	UI_PANELEX * pPanel = UICastToPanel(pPlayerMatchDialog);
	UI_PANELEX * pCurTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
	ASSERTV_RETURN(pCurTab, "UIDoListSelf: pCurTab is NULL. Contact Kevin Klemmick");

	UI_COMPONENT *pComp = UIComponentFindChildByName(pCurTab, "description edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyDescLbl = UICastToLabel(pComp);

	int nMaxMembers = 1;
	int nMinLevel = 1;
	int nMaxLevel = 99;

	c_SendPlayerAdvertiseLFG(pPartyDescLbl->m_Line.GetText(), nMaxMembers, nMinLevel, nMaxLevel );		

	UIPlayerMatchDialogContextChange(PMM_PLAYER);
}

void UIDoCreateMatch(void)
{
	UI_COMPONENT *pPlayerMatchDialog = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	if (!pPlayerMatchDialog || !UIComponentGetVisible(pPlayerMatchDialog))
		return;

	UI_PANELEX * pPanel = UICastToPanel(pPlayerMatchDialog);
	UI_PANELEX * pCurTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
	ASSERTV_RETURN(pCurTab, "UIDoListSelf: pCurTab is NULL. Contact Kevin Klemmick");
/*
	UI_COMPONENT *pComp = UIComponentFindChildByName(pCurTab, "description edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyDescLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pPlayerMatchDialog, "max players edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyMaxMembersLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pPlayerMatchDialog, "level min edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyMinLevelLbl = UICastToLabel(pComp);

	pComp = UIComponentFindChildByName(pPlayerMatchDialog, "level max edit");
	if (!pComp)
		return;
	UI_LABELEX *pPartyMaxLevelLbl = UICastToLabel(pComp);

	int nMaxMembers = ((pPartyMaxMembersLbl->m_Line.HasText())? PStrToInt(pPartyMaxMembersLbl->m_Line.GetText()) : 0);
	int nMinLevel = ((pPartyMinLevelLbl->m_Line.HasText())? PStrToInt(pPartyMinLevelLbl->m_Line.GetText()) : 0);
	int nMaxLevel = ((pPartyMaxLevelLbl->m_Line.HasText())? PStrToInt(pPartyMaxLevelLbl->m_Line.GetText()) : 0);

	c_SendPartyAdvertise(pPartyDescLbl->m_Line.GetText(), nMaxMembers, nMinLevel, nMaxLevel );		
*/
	GAME_SUBTYPE eGametype = GAME_SUBTYPE_PVP_CTF;
	UI_COMPONENT *pGametype = UIComponentFindChildByName(pCurTab, "gametype_combo");
	if (pGametype)
	{
		UI_COMBOBOX * pCombo = UICastToComboBox(pGametype);
		if (pCombo)
		{
			eGametype = (GAME_SUBTYPE) UIListBoxGetSelectedData(pCombo->m_pListBox);
		}
	}	

	MSG_CCMD_CREATE_PVP_GAME msg;
	msg.ePvPGameType = eGametype;
	c_SendMessage(CCMD_CREATE_PVP_GAME, &msg);

	UIPlayerMatchDialogContextChange(PMM_MATCH);
}

void UIDoJoinParty(void)
{
	UIDoLeaveParty();

	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();

	if (pParty)
	{
		CHAT_REQUEST_MSG_JOIN_PARTY joinMsg;
		joinMsg.TagChannel = pParty->m_nPartyID;
		c_ChatNetSendMessage(
			&joinMsg,
			USER_REQUEST_JOIN_PARTY );
	}
}

void UIAttemptJoinParty(void)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();

	if (pParty)
	{	
		MSG_CCMD_PARTY_JOIN_ATTEMPT tMessage;
		c_SendMessage(CCMD_PARTY_JOIN_ATTEMPT, &tMessage);
	}
}

void UIDoJoinMatch(void)
{
	UI_PARTY_DESC *pParty = sGetCurrentMatchDesc();
	if (pParty && pParty->m_idGame != 0)
	{
		MSG_CCMD_WARP_TO_GAME_INSTANCE message;
		message.nTypeOfInstance = GAME_INSTANCE_PVP;
		message.idGameToWarpTo = pParty->m_idGame;
		message.m_nLevelDef = pParty->m_nLevelDef;
		c_SendMessage(CCMD_WARP_TO_GAME_INSTANCE, &message);
	}
}

void UIDoWhisper(void)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();
	if (pParty && pParty->m_szPartyName)
		c_PlayerSetupWhisperTo(pParty->m_szPartyName);
}

void UIDoAutomatch(void)
{
	// KCK TODO:
}

void UIDoAutomatchDuel(void)
{
	MSG_CCMD_DUEL_AUTOMATED_SEEK msg;
	c_SendMessage(CCMD_DUEL_AUTOMATED_SEEK, &msg);
}

void UIDoInvite(void)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();
	if (pParty && pParty->m_szPartyName)
		c_PlayerInvite(pParty->m_szPartyName);
}

void UIDoChallenge(void)
{
	UI_PARTY_DESC *pParty = sGetCurrentPartyDesc();
	if (pParty && pParty->m_szPartyName && pParty->m_nCurrentPlayers > 0)
	{
		MSG_CCMD_DUEL_INVITE msg;
		msg.guidOpponent = pParty->m_tPartyMembers[0].m_idPlayer;
		c_SendMessage(CCMD_DUEL_INVITE, &msg);
	}
}

//----------------------------------------------------------------------------
// Context button 1
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyAction1BtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	UI_PARTY_DESC * pParty = sUIPartyPanelGetMyPartyDesc();
	BOOL bInParty = ( (c_PlayerGetPartyId() != INVALID_CHANNELID) ||
					  (c_PlayerGetPartyMemberCount() >= 2) );

	if (s_CurrentPlayerMatchMode == PMM_PARTY)
	{
		if (bInParty && pParty && !pParty->m_bForPlayerAdvertisementOnly )
			UIDoLeaveParty();										// Already in a party, so we should leave it.
		else
			UIAttemptJoinParty();
		UIPlayerMatchDialogContextChange(s_CurrentPlayerMatchMode);
	} 
	else if (s_CurrentPlayerMatchMode == PMM_PLAYER)
	{
		if (bInParty && pParty && pParty->m_bForPlayerAdvertisementOnly )
		{
			UIDoLeaveParty();										// Already in a party, so we should leave it.
			UIPlayerMatchDialogContextChange(s_CurrentPlayerMatchMode);
		}
		else
			UIPlayerMatchDialogContextChange(PMM_LIST_SELF);		// Goto List Self mode
	}
	else if (s_CurrentPlayerMatchMode == PMM_MATCH)
	{
		UIDoJoinMatch();
	}
	else if (s_CurrentPlayerMatchMode == PMM_PARTY_CREATE || 
			 s_CurrentPlayerMatchMode == PMM_LIST_SELF ||
			 s_CurrentPlayerMatchMode == PMM_MATCH_CREATE ||
			 s_CurrentPlayerMatchMode == PMM_AUTOMATCH)
	{
		// Cancel action. Return to previous context/tab
		UIPlayerMatchDialogContextChange(s_LastTab);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Context button 2
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyAction2BtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	UI_PARTY_DESC * pParty = sUIPartyPanelGetMyPartyDesc();
	BOOL bInParty = ( (c_PlayerGetPartyId() != INVALID_CHANNELID) ||
		(c_PlayerGetPartyMemberCount() >= 2) );

	if (s_CurrentPlayerMatchMode == PMM_PARTY)
	{
		if (bInParty && pParty && !pParty->m_bForPlayerAdvertisementOnly)
		{
			// KCK TODO! Challenge other party
		}
		else
			UIPlayerMatchDialogContextChange(PMM_PARTY_CREATE);
	}
	if (s_CurrentPlayerMatchMode == PMM_PLAYER)
	{
		if (bInParty && pParty && !pParty->m_bForPlayerAdvertisementOnly)
			UIDoInvite();
		else
			UIDoChallenge();
	}
	if (s_CurrentPlayerMatchMode == PMM_MATCH)
	{
		UIPlayerMatchDialogContextChange(PMM_MATCH_CREATE);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Context button 3
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyAction3BtnOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (s_CurrentPlayerMatchMode == PMM_PARTY_CREATE)
		UIDoCreateParty();
	else if (s_CurrentPlayerMatchMode == PMM_LIST_SELF)
		UIDoListSelf();
	else if (s_CurrentPlayerMatchMode == PMM_MATCH_CREATE)
		UIDoCreateMatch();
	else if (s_CurrentPlayerMatchMode == PMM_AUTOMATCH)
		UIDoAutomatch();
	else if (s_CurrentPlayerMatchMode == PMM_MATCH)
		UIPlayerMatchDialogContextChange(PMM_AUTOMATCH);
	else if (s_CurrentPlayerMatchMode == PMM_PLAYER)
		UIDoAutomatchDuel();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Main Display function for the Player Matching Dialog.
// Mostly paints the party/player/match lists
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pPlayerMatchDialog = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	if (!pPlayerMatchDialog || !UIComponentGetVisible(pPlayerMatchDialog))
		return UIMSG_RET_NOT_HANDLED; // possible it hasn't loaded yet

	UI_PANELEX * pPanel = UICastToPanel(pPlayerMatchDialog);
	ASSERTV_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnPaint: pPanel is NULL. Contact Kevin Klemmick");

	PlayerMatchModes	nMode = (PlayerMatchModes) pPanel->m_nCurrentTab;
	if (nMode >= PMM_PARTY_CREATE)
		return UIMSG_RET_HANDLED;

	UI_PANELEX * pTabs = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
	ASSERTV_RETVAL(pTabs, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnPaint: pTabs is NULL. Contact Kevin Klemmick");

	UIComponentRemoveAllElements(component);

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
	ASSERTV_RETVAL(pFont, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnPaint: pFont is NULL. Contact Kevin Klemmick");

	UI_COMPONENT *pLabelDesc	= UIComponentFindChildByName(pTabs, "header description");
	ASSERTV_RETVAL(pLabelDesc, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnPaint: pLabelDesc is NULL. Contact Kevin Klemmick");

	UI_COMPONENT *pLabelName	= UIComponentFindChildByName(pTabs, "header name");
	UI_COMPONENT *pLabelSize	= UIComponentFindChildByName(pTabs, "header size");
	UI_COMPONENT *pLabelType	= UIComponentFindChildByName(pTabs, "header type");
	UI_COMPONENT *pLabelPlayers	= UIComponentFindChildByName(pTabs, "header players");

	// Find "currently selected" highlight bar
	UI_COMPONENT* pHilightBar = UIComponentFindChildByName(pTabs, "list highlight");
	ASSERT_RETVAL(pHilightBar, UIMSG_RET_NOT_HANDLED);
	if (pHilightBar)
	{
		pHilightBar->m_ScrollPos.m_fY = component->m_ScrollPos.m_fY;
		UIComponentSetVisible(pHilightBar, FALSE);
	}

	// Find pixel locations of standard columns
	UI_RECT rectDescCol(pLabelDesc->m_Position.m_fX,	0.0f, pLabelDesc->m_Position.m_fX + pLabelDesc->m_fWidth,	component->m_fHeight);

	UI_POSITION pos;
	UI_SIZE size;
	pos.m_fY = 0.0f;
	int nFontSize = UIComponentGetFontSize(component, pFont);
	size.m_fHeight = (float)nFontSize;
	const float fLineSpacing = (nFontSize * UIGetScreenToLogRatioY(component->m_bNoScaleFont));

	// Populate the list
	UI_PARTY_DESC *pParty = gpUIPartyList;
	int	nCount = gnPartyListCount;
	if (nMode == PMM_MATCH)
	{
		pParty = gpUIMatchList;
		nCount = gnMatchListCount;
	}
	for (int i=0; pParty && i < nCount; i++,pParty++)
	{
		if (pParty->m_bForPlayerAdvertisementOnly && nMode != PMM_PLAYER)
			continue;
		if (!pParty->m_bForPlayerAdvertisementOnly)
		{
			if (nMode == PMM_PLAYER)
				continue;
			if (pParty->m_nMatchType && nMode != PMM_MATCH)
				continue;
			if (!pParty->m_nMatchType && nMode == PMM_MATCH)
				continue;
		}
		if (pParty->m_nPartyID != INVALID_ID)
		{
			DWORD dwFontColor = GFXCOLOR_WHITE;

			if (pParty->m_nPartyID == (int)component->m_dwData)
			{
				dwFontColor = GetFontColor(FONTCOLOR_FSORANGE);

				if (pHilightBar)
				{
					if (pParty->m_pDescElement)
						pHilightBar->m_Position.m_fY = pos.m_fY;
					UIComponentSetVisible(pHilightBar, TRUE);
				}
			}

			// write description
			pos.m_fX = ( rectDescCol.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
			size.m_fWidth = ( rectDescCol.m_fX2 - rectDescCol.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
			pParty->m_pDescElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, pParty->m_szPartyDesc, pos, dwFontColor, &rectDescCol, UIALIGN_LEFT, &size, FALSE);

			// write name
			if (pLabelName)
			{
				UI_RECT rectNameCol(pLabelName->m_Position.m_fX,	0.0f, pLabelName->m_Position.m_fX + pLabelName->m_fWidth,	component->m_fHeight);
				pos.m_fX = ( rectNameCol.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				size.m_fWidth = ( rectNameCol.m_fX2 - rectNameCol.m_fX1  ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				pParty->m_pNameElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, pParty->m_szPartyName, pos, dwFontColor, &rectNameCol, UIALIGN_LEFT, &size, FALSE);
			}

			// write size
			if (pLabelSize)
			{
				UI_RECT rect(pLabelSize->m_Position.m_fX,	0.0f, pLabelSize->m_Position.m_fX + pLabelSize->m_fWidth,	component->m_fHeight);
				pos.m_fX = ( rect.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				size.m_fWidth = ( rect.m_fX2 - rect.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				WCHAR		buf[32];
				PStrPrintf(buf, 32, L"%d", pParty->m_nCurrentPlayers);
				pParty->m_pSizeElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, buf, pos, dwFontColor, &rect, UIALIGN_LEFT, &size, FALSE);
			}

			// write type
			if (pLabelType)
			{
				UI_RECT rect(pLabelType->m_Position.m_fX,	0.0f, pLabelType->m_Position.m_fX + pLabelType->m_fWidth,	component->m_fHeight);
				pos.m_fX = ( rect.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				size.m_fWidth = ( rect.m_fX2 - rect.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				WCHAR		buf[32];
				PStrPrintf(buf, 32, L"%d", pParty->m_nMatchType);			// KCK TODO: Match type from Strings!
				pParty->m_pTypeElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, buf, pos, dwFontColor, &rect, UIALIGN_LEFT, &size, FALSE);
			}

			// write players
			if (pLabelPlayers)
			{
				UI_RECT rect(pLabelPlayers->m_Position.m_fX,	0.0f, pLabelPlayers->m_Position.m_fX + pLabelPlayers->m_fWidth,	component->m_fHeight);
				pos.m_fX = ( rect.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				size.m_fWidth = ( rect.m_fX2 - rect.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
				WCHAR		buf[32];	
				// KCK TODO: Add proper player levels for each team
				PStrPrintf(buf, 32, L"%d/%d %d/%d", pParty->m_nCurrentPlayers, pParty->m_nMaxPlayers, pParty->m_nCurrentPlayers, pParty->m_nMaxPlayers);
				pParty->m_pPlayersElement = UIComponentAddTextElement(component, NULL, pFont, nFontSize, buf, pos, dwFontColor, &rect, UIALIGN_LEFT, &size, FALSE);
			}

			pos.m_fY += fLineSpacing;
		}
	}

	UI_COMPONENT *pScrollBar = UIComponentIterateChildren(UIComponentGetByEnum(UICOMP_PARTYLIST), NULL, UITYPE_SCROLLBAR, FALSE);
	if (pScrollBar)
	{	
		UI_SCROLLBAR *pScroll = UICastToScrollBar(pScrollBar);
		pScroll->m_fMin = 0;
		pScroll->m_fMax = pos.m_fY - component->m_fHeight;
		if (pScroll->m_fMax < 0.0f)
		{
			pScroll->m_fMax = 0.0f;
			UIComponentSetVisible(pScroll, FALSE);
		}
		else
		{
			UIComponentSetActive(pScroll, TRUE);
		}
		component->m_fScrollVertMax = pScroll->m_fMax;
		component->m_ScrollPos.m_fY = MIN(component->m_ScrollPos.m_fY, component->m_fScrollVertMax);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// get mouse relative to the component
	x -= pos.m_fX;
	y -= pos.m_fY;
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);
	// compensate for scrolling
	x += UIComponentGetScrollPos(component).m_fX;
	y += UIComponentGetScrollPos(component).m_fY;

	UI_COMPONENT *pPlayerMatchDialog = UIComponentGetByEnum(UICOMP_PLAYERMATCH_PANEL);
	ASSERTV_RETVAL(pPlayerMatchDialog, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnLButtonDown: pPlayerMatchDialog is NULL. Contact Kevin Klemmick");
	UI_PANELEX * pPanel = UICastToPanel(pPlayerMatchDialog);
	ASSERTV_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnLButtonDown: pPanel is NULL. Contact Kevin Klemmick");
	UI_PANELEX * pCurTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
	ASSERTV_RETVAL(pPanel, UIMSG_RET_NOT_HANDLED, "UIPlayerMatchOnLButtonDown: pCurTab is NULL. Contact Kevin Klemmick");

	UI_COMPONENT* pHilightBar = UIComponentFindChildByName(pCurTab, "list highlight");
	ASSERT_RETVAL(pHilightBar, UIMSG_RET_NOT_HANDLED);

	int i = 0;
	UI_RECT rectElement;
	UI_PARTY_DESC *pParty = gpUIPartyList;
	int	nCount = gnPartyListCount;
	if (s_CurrentPlayerMatchMode == PMM_MATCH)
	{
		pParty = gpUIMatchList;
		nCount = gnMatchListCount;
	}
	while (pParty && i < nCount)
	{
		//	don't check removed entries...
		if( pParty->m_nPartyID == -1 ) {
			pParty++;
			i++;
			continue;
		}

		if (UIElementCheckBounds(pParty->m_pNameElement, x, y) ||
			UIElementCheckBounds(pParty->m_pDescElement, x, y) || 
			UIElementCheckBounds(pParty->m_pSizeElement, x, y) ||
			UIElementCheckBounds(pParty->m_pTypeElement, x, y) ||
			UIElementCheckBounds(pParty->m_pPlayersElement, x, y) )
		{
			component->m_dwData = (DWORD)pParty->m_nPartyID;
			UIPartyRequestMembers(pParty->m_nPartyID);

			if (pHilightBar)
			{
				pHilightBar->m_Position.m_fY = pParty->m_pDescElement->m_Position.m_fY;
				UIComponentSetVisible(pHilightBar, TRUE);
			}

			UIPlayerMatchDialogContextChange(s_CurrentPlayerMatchMode);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		pParty++;
		i++;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerMatchOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	static BOOL bColorOn = FALSE;
	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos) && !bColorOn)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// get mouse relative to the component
	x -= pos.m_fX;
	y -= pos.m_fY;
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);

	// compensate for scrolling
	x += UIComponentGetScrollPos(component).m_fX;
	y += UIComponentGetScrollPos(component).m_fY;

	int i = 0;
	UI_RECT rectElement;
	UI_PARTY_DESC *pParty = gpUIPartyList;
	int	nCount = gnPartyListCount;
	if (s_CurrentPlayerMatchMode == PMM_MATCH)
	{
		pParty = gpUIMatchList;
		nCount = gnMatchListCount;
	}
	while (pParty && i < nCount)
	{
		//	don't check removed entries...
		if (pParty->m_nPartyID == -1) 
		{
			pParty++;
			i++;
			continue;
		}

		DWORD dwColor = GFXCOLOR_WHITE;
		bColorOn = false;
		if (component->m_dwData == (DWORD)pParty->m_nPartyID)
		{
			dwColor = GetFontColor(FONTCOLOR_FSORANGE);
			bColorOn = true;
		}
		else if (UIElementCheckBounds(pParty->m_pNameElement, x, y) ||
				 UIElementCheckBounds(pParty->m_pDescElement, x, y) || 
				 UIElementCheckBounds(pParty->m_pSizeElement, x, y) ||
				 UIElementCheckBounds(pParty->m_pTypeElement, x, y) ||
				 UIElementCheckBounds(pParty->m_pPlayersElement, x, y) )
		{
			dwColor = GFXCOLOR_YELLOW;
			bColorOn = true;
		}

		if (pParty->m_pNameElement)
			pParty->m_pNameElement->m_dwColor =	dwColor;
		if (pParty->m_pDescElement)
			pParty->m_pDescElement->m_dwColor = dwColor;
		if (pParty->m_pSizeElement)
			pParty->m_pSizeElement->m_dwColor = dwColor;
		if (pParty->m_pTypeElement)
			pParty->m_pTypeElement->m_dwColor = dwColor;
		if (pParty->m_pPlayersElement)
			pParty->m_pPlayersElement->m_dwColor = dwColor;

		pParty++;
		i++;
	}

	UISetNeedToRender(component);
	return UIMSG_RET_HANDLED;
}







//----------------------------------------------------------------------------
// PVP Scoreboard Stuff
//----------------------------------------------------------------------------

static DWORD sgdwRefreshScoreboardTicket = INVALID_ID;

//----------------------------------------------------------------------------
// PVP Scoreboard Paint Function
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScoreboardOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	PvPGameData		tGameData;

	if (c_PvPGetGameData(&tGameData) == TRUE)
	{
		WCHAR	szBuf[256];

		UIComponentRemoveAllElements(component);
		UIComponentAddFrame(component);

		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
		ASSERTV_RETVAL(pFont, UIMSG_RET_NOT_HANDLED, "UIScoreboardOnPaint: pFont is NULL. Contact Kevin Klemmick");

		UI_COMPONENT *pComponent	= UIComponentFindChildByName(component, "score l");
		ASSERTV_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED, "UIScoreboardOnPaint: pComponent is NULL. Contact Kevin Klemmick");
		PStrPrintf(szBuf, 256, L"%d", tGameData.tTeamData[0].nScore);
		UILabelSetText(pComponent, szBuf);
		pComponent	= UIComponentFindChildByName(component, "score r");
		ASSERTV_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED, "UIScoreboardOnPaint: pComponent is NULL. Contact Kevin Klemmick");
		PStrPrintf(szBuf, 256, L"%d", tGameData.tTeamData[1].nScore);
		UILabelSetText(pComponent, szBuf);
		pComponent	= UIComponentFindChildByName(component, "timer");
		ASSERTV_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED, "UIScoreboardOnPaint: pComponent is NULL. Contact Kevin Klemmick");
		LanguageGetTimeString(szBuf, 256, tGameData.nSecondsRemainingInMatch);
		UILabelSetText(pComponent, szBuf);

		// Check to see if we've got player data headers, and fill those columns if so.
		UI_COMPONENT *pHeader	= UIComponentFindChildByName(component, "ctf_hdr_0_player");
		if (pHeader)
		{
			UI_POSITION pos;
			UI_SIZE size;
			int nFontSize = UIComponentGetFontSize(component, pFont);
			size.m_fHeight = (float)nFontSize;
			const float fLineSpacing = (nFontSize * UIGetScreenToLogRatioY(component->m_bNoScaleFont));

			// KCK TODO: Make this work
			for (int t=0; t<tGameData.nNumTeams; t++)
			{
				#define MAX_LABEL_LEN	64
				char	szPlayerKey[MAX_LABEL_LEN];
				char	szKillKey[MAX_LABEL_LEN];
				char	szFlagKey[MAX_LABEL_LEN];

				PStrPrintf(szPlayerKey, MAX_LABEL_LEN, "ctf_hdr_%d_player", t);
				PStrPrintf(szKillKey, MAX_LABEL_LEN, "ctf_hdr_%d_kills", t);
				PStrPrintf(szFlagKey, MAX_LABEL_LEN, "ctf_hdr_%d_flag", t);

				UI_COMPONENT *pPlayerHdr	= UIComponentFindChildByName(component, szPlayerKey);
				UI_COMPONENT *pKillsHdr	= UIComponentFindChildByName(component, szKillKey);
				UI_COMPONENT *pFlagHdr	= UIComponentFindChildByName(component, szFlagKey);
				pos.m_fY = pHeader->m_Position.m_fY + pHeader->m_fHeight + 5.0f;

				for (int p=0; p<tGameData.tTeamData[t].nNumPlayers; p++)
				{
					PvPPlayerData *	pPlayerData = &(tGameData.tTeamData[t].tPlayerData[p]);

					// KCK: Not sure how we know if we have a player in this slot or not other than an empty name
					if (pPlayerData->szName[0] != 0)
					{
						DWORD dwFontColor = GFXCOLOR_WHITE;

						// write name
						if  (pPlayerHdr)
						{
							UI_RECT rect(pPlayerHdr->m_Position.m_fX, 0.0f, pPlayerHdr->m_Position.m_fX + pPlayerHdr->m_fWidth,	component->m_fHeight);
							pos.m_fX = ( rect.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
							size.m_fWidth = ( rect.m_fX2 - rect.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
							UIComponentAddTextElement(component, NULL, pFont, nFontSize, pPlayerData->szName, pos, dwFontColor, &rect, UIALIGN_LEFT, &size, FALSE);
						}

						// write kills
						if (pKillsHdr)
						{
							UI_RECT rect(pKillsHdr->m_Position.m_fX, 0.0f, pKillsHdr->m_Position.m_fX + pKillsHdr->m_fWidth,	component->m_fHeight);
							pos.m_fX = ( rect.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
							size.m_fWidth = ( rect.m_fX2 - rect.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
							WCHAR		buf[32];	
							PStrPrintf(buf, 32, L"%d", pPlayerData->nPlayerKills);
							UIComponentAddTextElement(component, NULL, pFont, nFontSize, buf, pos, dwFontColor, &rect, UIALIGN_LEFT, &size, FALSE);
						}

						// write flags
						if (pFlagHdr)
						{
							UI_RECT rect(pFlagHdr->m_Position.m_fX,	0.0f, pFlagHdr->m_Position.m_fX + pFlagHdr->m_fWidth,	component->m_fHeight);
							pos.m_fX = ( rect.m_fX1 + 5 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
							size.m_fWidth = ( rect.m_fX2 - rect.m_fX1 ) / UIGetScreenToLogRatioX(component->m_bNoScaleFont);
							WCHAR		buf[32];	
							PStrPrintf(buf, 32, L"%d", pPlayerData->nFlagCaptures);
							UIComponentAddTextElement(component, NULL, pFont, nFontSize, buf, pos, dwFontColor, &rect, UIALIGN_LEFT, &size, FALSE);
						}

						pos.m_fY += fLineSpacing;
					}
				}
			}
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
// Refresh list event callback
//----------------------------------------------------------------------------
static void sRefreshScoreboard(
	GAME*,
	const CEVENT_DATA&,
	DWORD)
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_SCOREBOARD_SHORT);
	if (component && UIComponentGetVisible(component))
		UIScoreboardOnPaint(component, 0, 0, 0);
	component = UIComponentGetByEnum(UICOMP_SCOREBOARD);
	if (component && UIComponentGetVisible(component))
		UIScoreboardOnPaint(component, 0, 0, 0);

	// KCK: Is there a way we can simply add a repeating event rather than re-adding this each time we call this trigger?
	// Cancel any event timers we had running
	if (sgdwRefreshScoreboardTicket != INVALID_ID)
		CSchedulerCancelEvent(sgdwRefreshScoreboardTicket);

	// Register a timed event to keep the scores updated
	TIME time = CSchedulerGetTime() + SCHEDULER_TIME_PER_SECOND;
	CEVENT_DATA nodata;
	sgdwRefreshScoreboardTicket = CSchedulerRegisterEvent(time, sRefreshScoreboard,	nodata);
}

//----------------------------------------------------------------------------
// PVP Scoreboard On Post Visible
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScoreboardOnPostVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// Cancel any event timers we had running
	if (sgdwRefreshScoreboardTicket != INVALID_ID)
		CSchedulerCancelEvent(sgdwRefreshScoreboardTicket);

	// Register a timed event to keep the scores updated
	TIME time = CSchedulerGetTime() + SCHEDULER_TIME_PER_SECOND;
	CEVENT_DATA nodata;
	sgdwRefreshScoreboardTicket = CSchedulerRegisterEvent(time, sRefreshScoreboard,	nodata);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// PVP Scoreboard On Post Invisible
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScoreboardOnPostInvisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// Cancel any event timers we had running
	if (sgdwRefreshScoreboardTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(sgdwRefreshScoreboardTicket);
	}

	// If we just hid the large scoreboard, make the short one visible
	UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_SCOREBOARD_SHORT);
	if (pComponent && pComponent != component)
	{
		UIComponentSetActive(pComponent, TRUE);
		UIComponentSetVisible(pComponent, TRUE);
	}

	return UIMSG_RET_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIReceiveTownInstanceList(
	LPVOID pArray,
	DWORD dwInstanceCount )
{
	MSG_SCMD_GAME_INSTANCE_UPDATE * instanceArray = (MSG_SCMD_GAME_INSTANCE_UPDATE*)pArray;
	REF( instanceArray );

	UI_COMPONENT *pTownInstanceList = UIComponentGetByEnum(UICOMP_TOWN_INSTANCE_COMBO);
	//	ASSERT_RETFALSE(pTownInstanceList);
	if (!pTownInstanceList)
		return FALSE;

	UI_COMBOBOX * pCombo = UICastToComboBox(pTownInstanceList);
	UITextBoxClear(pCombo->m_pListBox);

	int nAreaID = -1;
	UNIT* pPlayer = UIGetControlUnit();
	if( pPlayer )
	{
		// KCK: Hellgate doesn't use Area ID, use  Definition Index instead
		if ( AppIsTugboat() )
			nAreaID = UnitGetLevelAreaID( pPlayer );
		if ( AppIsHellgate() )
			nAreaID = UnitGetLevelDefinitionIndex( pPlayer );
	}
	int nSelectionIndex = 0;//nZoneSelected;
	int nIndexAll = -1;
	int nIndices = 0;
	int nPVPWorld = 0;
	if( pPlayer && PlayerIsInPVPWorld( pPlayer ) )
	{
		nPVPWorld = 1;
	}
	else if( pPlayer && UnitGetLevel( pPlayer ) && LevelGetPVPWorld( UnitGetLevel( pPlayer ) ) )
	{
		nPVPWorld = 1;
	}
	for( DWORD i = 0; i < dwInstanceCount; i++ )
	{
		if( instanceArray[i].idLevelDef == nAreaID &&
			instanceArray[i].nPVPWorld == nPVPWorld )
		{
			WCHAR Str[MAX_XML_STRING_LENGTH];
			if ( AppIsTugboat() )
			{
				MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = (MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS, instanceArray[i].idLevelDef );
				PStrPrintf(Str, arrsize(Str), L"%d - %s", instanceArray[i].nInstanceNumber , StringTableGetStringByIndex( pLevelArea->nLevelAreaDisplayNameStringIndex ) );
			}
			else if ( AppIsHellgate() && pPlayer )
			{
				LEVEL* pLevel = UnitGetLevel( pPlayer );
				if ( pLevel )
				{
					PStrPrintf(Str, arrsize(Str), L"%d - %s", instanceArray[i].nInstanceNumber, StringTableGetStringByIndex(pLevel->m_pLevelDefinition->nLevelDisplayNameStringIndex));
				}
			}
			if( instanceArray[i].idGame == GameGetId( UnitGetGame( pPlayer ) ) )
			{
				nSelectionIndex = nIndices;
			}
			nIndices++;
			UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)instanceArray[i].idGame);
		}

	}

	if( nSelectionIndex >= nIndices )
	{
		nSelectionIndex = 0;
	}
	nSelectionIndex = (nSelectionIndex < 0) ? nIndexAll : nSelectionIndex;

	UIComboBoxSetSelectedIndex(pCombo, nSelectionIndex, TRUE, FALSE);


	if( nIndices < 2 )
	{
		UIComponentSetActive( pCombo, FALSE );
		UIComponentSetVisible( pCombo, FALSE );
	}
	else
	{
		UIComponentSetActive( pCombo, TRUE );
		UIComponentSetVisible( pCombo, TRUE );
	}

	UIComponentHandleUIMessage(pTownInstanceList, UIMSG_PAINT, 0, 0);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIReceivePVPInstanceList(
	LPVOID pArray,
	DWORD instanceCount )
{
	MSG_SCMD_GAME_INSTANCE_UPDATE * instanceArray = (MSG_SCMD_GAME_INSTANCE_UPDATE*)pArray;

	UI_COMPONENT *pPartyList = UIComponentGetByEnum(UICOMP_PARTYLIST);
	BOOL bPartyFound = FALSE;

	// KCK: We're going to use the UI_PARTY_DESC to store this info, since it makes coding the various player
	// matching tabs more consistent.
	gpUIMatchList = (UI_PARTY_DESC*)REALLOCZ(NULL, gpUIMatchList, sizeof(UI_PARTY_DESC) * instanceCount);
	gnMatchListCount = instanceCount;

	for (UINT i = 0; i < instanceCount; ++i)
	{
		UI_PARTY_DESC * tParty = &gpUIMatchList[i];
		tParty->m_nPartyID = i+1;
		PStrCopy(tParty->m_szPartyDesc, L"KEVIN, FIX THIS", MAX_PARTY_STRING);
		tParty->m_idGame = instanceArray[i].idGame;
		tParty->m_nInstanceNumber = instanceArray[i].nInstanceNumber;
		tParty->m_nLevelDef = instanceArray[i].idLevelDef;
		tParty->m_nMatchType = 1; // KCK Need to get this from somewhere.

		if ((int)pPartyList->m_dwData == tParty->m_nPartyID)
			bPartyFound = TRUE;
	}

	if (!bPartyFound && s_CurrentPlayerMatchMode == PMM_MATCH)
		pPartyList->m_dwData = (DWORD)INVALID_ID;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIInstanceListReceiveInstances(
	GAME_INSTANCE_TYPE eInstanceType,
	LPVOID pArray,
	DWORD dwInstanceCount )
{
	MSG_SCMD_GAME_INSTANCE_UPDATE * instanceArray = (MSG_SCMD_GAME_INSTANCE_UPDATE*)pArray;
	REF( instanceArray );

	// KCK: Kinda funky, but as we're using this same list for different uses, decide where to
	// send this data
	if (eInstanceType == GAME_INSTANCE_TOWN)
		return UIReceiveTownInstanceList(pArray, dwInstanceCount);
	if (eInstanceType == GAME_INSTANCE_PVP)
		return UIReceivePVPInstanceList(pArray, dwInstanceCount);

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUpdateWaypointButtons(
	UI_COMPONENT *pWaypoints)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return;
	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return;

	// create list of levels I can warp to
	const WCHAR * uszLevelNames[MAX_WAYPOINT_BLOCKS];
	unsigned int nListLength = 0;
	WaypointCreateListForPlayer(pGame, pPlayer, &uszLevelNames[0], NULL, &nListLength);

	UINT nButtons = 0;
	UI_COMPONENT *pBtn = NULL;
	while ((pBtn = UIComponentIterateChildren(pWaypoints, pBtn, UITYPE_BUTTON, FALSE)) != NULL)
	{
		if (pBtn->m_dwParam > 0)
		{
			nButtons++;
		}
	}

	if (nListLength <= nButtons)
		pWaypoints->m_dwData = 0;
	else
		if (pWaypoints->m_dwData > nListLength - nButtons)
			pWaypoints->m_dwData = nListLength - nButtons;

	int nOffset = (int)pWaypoints->m_dwData;
	UI_COMPONENT *pUpButton = UIComponentFindChildByName(pWaypoints, "waypoint scroll button up");
	if (pUpButton)
	{
		UIComponentSetActive(pUpButton, nOffset > 0);
	}

	int nNumAdded = 0;
	pBtn = NULL;
	while ((pBtn = UIComponentIterateChildren(pWaypoints, pBtn, UITYPE_BUTTON, FALSE)) != NULL)
	{
		if (pBtn->m_dwParam > 0)
		{
			int nIndex = nOffset + (int)pBtn->m_dwParam - 1;
			if (nIndex < (int)nListLength)
			{
				UIComponentSetActive(pBtn, TRUE);
				nNumAdded++;
				if (pBtn->m_pFirstChild->m_eComponentType == UITYPE_LABEL)
				{
					UILabelSetText(pBtn->m_pFirstChild, uszLevelNames[nIndex]);
				}
			}
			else
			{
				UIComponentSetVisible(pBtn, FALSE);
			}
		}
	}

	UI_COMPONENT *pDownButton = UIComponentFindChildByName(pWaypoints, "waypoint scroll button down");
	if (pDownButton)
	{
		UIComponentSetActive(pDownButton, nNumAdded + nOffset < (int)nListLength);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowWaypointsScreen()
{
	UI_COMPONENT * component = UIComponentGetByEnum(UICOMP_KIOSK);
	if (!component)
	{
		return;
	}

	UIStopSkills();
	c_PlayerClearAllMovement(AppGetCltGame());

	UI_PANELEX *pKiosk = UICastToPanel(component);

	UIComponentActivate(pKiosk, TRUE);

	pKiosk->m_dwData = 0;
	sUpdateWaypointButtons(pKiosk);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWaypointScrollUpOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pParent = component->m_pParent;
	ASSERT_RETVAL(pParent, UIMSG_RET_NOT_HANDLED);

	if (pParent->m_dwData > 0)
		pParent->m_dwData--;

	sUpdateWaypointButtons(pParent);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWaypointScrollDownOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pParent = component->m_pParent;
	ASSERT_RETVAL(pParent, UIMSG_RET_NOT_HANDLED);

	pParent->m_dwData++;

	sUpdateWaypointButtons(pParent);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWaypointsOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentCheckBounds(component) || !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	int nDelta = ((int)lParam > 0 ? -1 : 1);

	if (nDelta > 0 || component->m_dwData > 0)	
		component->m_dwData += nDelta;

	sUpdateWaypointButtons(component);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWaypointsTransportOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIButtonOnButtonDown(component, msg, wParam, lParam);
	if (!ResultIsHandled(eResult))
	{
		return eResult;
	}

	UI_PANELEX *pKiosk = UICastToPanel(UIComponentGetByEnum(UICOMP_KIOSK));

	UI_COMPONENT *pWaypointsPanel = UIPanelGetTab(pKiosk, 0);
	ASSERT_RETVAL(pWaypointsPanel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pPanel = UIComponentFindChildByName(pWaypointsPanel, "waypoints list");
	if (!pPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pChild = pPanel->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_dwParam != 0)
		{
			if (pChild->m_eComponentType == UITYPE_LABEL)
			{
				UI_LABELEX *pLabel = UICastToLabel(pChild);
				if (pLabel->m_bSelected)
				{
					c_WaypointTryOperate( AppGetCltGame(), UIGetControlUnit(), (unsigned int)pLabel->m_dwParam-1 );
					UIComponentSetVisible(pKiosk, FALSE);
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
		}
		pChild = pChild->m_pNextSibling;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWaypointsWaypointOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PANELEX *pKiosk = UICastToPanel(UIComponentGetByEnum(UICOMP_KIOSK));

	if (component->m_dwParam != 0)
	{
		int nOffset = (int)pKiosk->m_dwData;
		unsigned int nWaypoint = (unsigned int)component->m_dwParam - 1 + nOffset;
		c_WaypointTryOperate( AppGetCltGame(), UIGetControlUnit(), nWaypoint );
		UIComponentHandleUIMessage(component, UIMSG_MOUSELEAVE, 0, 0);
		UIComponentActivate(pKiosk, FALSE);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICloseParent(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYDOWN ||
		msg == UIMSG_KEYCHAR ||
		msg == UIMSG_KEYUP)
	{
		if (wParam != VK_ESCAPE)
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

	if (component && component->m_pParent)
	{
		UIComponentActivate(component->m_pParent, FALSE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUpdateAnchorstoneButtons(
	UI_COMPONENT *pAnchorstones)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return;
	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return;

	UINT nButtons = 0;
	UI_COMPONENT *pBtn = NULL;
	while ((pBtn = UIComponentIterateChildren(pAnchorstones, pBtn, UITYPE_BUTTON, FALSE)) != NULL)
	{
		if (pBtn->m_dwParam > 0)
		{
			nButtons++;
		}
	}

	
	LEVEL* pLevel = UnitGetLevel( pPlayer );
	if( !pLevel )
	{
		return;
	}
	
	MYTHOS_ANCHORMARKERS::cAnchorMarkers* pMarkers = LevelGetAnchorMarkers( pLevel );
	/*int nAnchorstones = pMarkers->GetAnchorCount();
	int nKnownAnchorstones = 0;
	int nKnownAnchors[100];
	for( int i = 0; i < nAnchorstones; i++ )
	{
		const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor = pMarkers->GetAnchorByIndex( i );
		if( pMarkers->HasAnchorBeenVisited( UIGetControlUnit(), pAnchor ) )
		{
			nKnownAnchors[nKnownAnchorstones] = i;
			nKnownAnchorstones++;
		}
	}*/


	const UNIT_DATA* pAnchorshrines[100];
	int nAnchorshrines = 0;
	int nNumObjectDefs = ExcelGetNumRows( NULL, DATATABLE_OBJECTS );	

	// go through all level defs and find the first level
	for (int i = 0 ; i < nNumObjectDefs; ++i)
	{
		const UNIT_DATA * pObjectData = ObjectGetData(pGame, i);
		if( pMarkers->HasAnchorBeenVisited( UIGetControlUnit(), i ) )
		{
			pAnchorshrines[nAnchorshrines] = pObjectData;
			nAnchorshrines++;
		}
	}


	// populate the window with the choices
	UI_COMPONENT *pChild = pAnchorstones->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_dwParam != 0)
		{
			if ( (int)pChild->m_dwParam <= nAnchorshrines)
			{
				//const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor = pMarkers->GetAnchorByIndex( nKnownAnchors[pChild->m_dwParam - 1] );
				//pChild->m_dwParam2 = nKnownAnchors[pChild->m_dwParam - 1];
				pChild->m_dwParam2 = pAnchorshrines[pChild->m_dwParam - 1]->wCode;
				if (pChild->m_eComponentType == UITYPE_LABEL)
				{
					const WCHAR *puszString = StringTableGetStringByIndexVerbose(pAnchorshrines[pChild->m_dwParam - 1]->nString, 0, 0, NULL, NULL);

					UILabelSetText( pChild, puszString );
				}
				UIComponentSetActive( pChild, TRUE );
			}
			else
			{
				UIComponentSetVisible( pChild, FALSE );
			}

		}
		pChild = pChild->m_pNextSibling;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowAnchorstoneScreen()
{
	UI_COMPONENT * component = UIComponentGetByEnum(UICOMP_ANCHORSTONE_PANEL);
	if (!component)
	{
		return;
	}
	UIStopSkills();
	c_PlayerClearAllMovement(AppGetCltGame());
	UIComponentActivate(component, TRUE);
	UI_TB_HideWorldMapScreen( TRUE );
	sUpdateAnchorstoneButtons(component->m_pFirstChild );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAnchorstoneButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIButtonOnButtonDown(component, msg, wParam, lParam);
	if (!ResultIsHandled(eResult))
	{
		return eResult;
	}

	int nIndex = (int)component->m_dwParam2;	

	
	MSG_CCMD_OPERATE_ANCHORSTONE anchormsg;
	anchormsg.nIndex = nIndex;
	anchormsg.nAction = 1;
	anchormsg.wObjectCode = (WORD)nIndex;
	c_SendMessage(CCMD_OPERATE_ANCHORSTONE, &anchormsg);

	UI_COMPONENT * pPanel = UIComponentGetByEnum(UICOMP_ANCHORSTONE_PANEL);
	if (!pPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentActivate(pPanel, FALSE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAnchorstoneLinkButtonDown(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIButtonOnButtonDown(component, msg, wParam, lParam);
	if (!ResultIsHandled(eResult))
	{
		return eResult;
	}


	MSG_CCMD_OPERATE_ANCHORSTONE anchormsg;
	anchormsg.nAction = 0;
	c_SendMessage(CCMD_OPERATE_ANCHORSTONE, &anchormsg);

	UI_COMPONENT * pPanel = UIComponentGetByEnum(UICOMP_ANCHORSTONE_PANEL);
	if (!pPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentActivate(pPanel, FALSE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAnchorstoneCancelButtonDown(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIButtonOnButtonDown(component, msg, wParam, lParam);
	if (!ResultIsHandled(eResult))
	{
		return eResult;
	}
	UI_COMPONENT * pPanel = UIComponentGetByEnum(UICOMP_ANCHORSTONE_PANEL);
	if (!pPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentActivate(pPanel, FALSE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUpdateRunegateButtons(
	UI_COMPONENT *pRunegates)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return;
	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return;

	UI_SCROLLBAR *pScrollbar = UICastToScrollBar( UIComponentFindChildByName( pRunegates, "runegate list scrollbar" ) );
	pRunegates = UIComponentFindChildByName( pRunegates, "scroll pane" );


	UINT nButtons = 0;
	UI_COMPONENT *pBtn = NULL;
	while ((pBtn = UIComponentIterateChildren(pRunegates, pBtn, UITYPE_BUTTON, FALSE)) != NULL)
	{
		if (pBtn->m_dwParam > 0)
		{
			nButtons++;
		}
	}


	LEVEL* pLevel = UnitGetLevel( pPlayer );
	if( !pLevel )
	{
		return;
	}
	UNITID idTrigger = UnitGetStat(pPlayer, STATS_TALKING_TO );
	UNIT *pTrigger = UnitGetById(pGame, idTrigger);
	if( !pTrigger )
	{
		return;
	}
	
	float fY = 0;
	//////////////////////////////////////////////////////////////////////////
	//Iterate the entire players inventory looking for keys.
	//////////////////////////////////////////////////////////////////////////

	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );		
	int nKnownRunegates = 0;
	UNIT* pKnownRunegateAreas[100];
	// only search on person ( need to make this search party members too!		
	UNIT * pItem = NULL;			
	while ((pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags )) != NULL)
	{
		if ( ObjectItemIsRequiredToUse( pTrigger, pItem ) )
		{				
			pKnownRunegateAreas[nKnownRunegates] = pItem;//MYTHOS_MAPS::GetLevelAreaID( pItem );
			nKnownRunegates++;
		}
	}	

	int nLastLevelArea = UnitGetStat( pPlayer, STATS_LAST_LEVEL_AREA );

	if( MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( nLastLevelArea ) ||
		!MYTHOS_LEVELAREAS::LevelAreaIsRuneStone( nLastLevelArea ))
	{
		nLastLevelArea = INVALID_ID;
	}
	int nMember = 0;
	int nActualMembers = 0;
	PGUID PartyMemberGuids[10];
	int nPartyLevelAreas[10];
	while (nMember < MAX_CHAT_PARTY_MEMBERS)
	{

		if (c_PlayerGetPartyMemberGUID(nMember) != INVALID_GUID)
		{
			int nArea = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
			BOOL bPVPWorld = c_PlayerGetPartyMemberPVPWorld(nMember, FALSE );
			if( nArea >= 0 &&
				bPVPWorld == PlayerIsInPVPWorld( pPlayer ) )
			{					
				if( MYTHOS_LEVELAREAS::LevelAreaIsRuneStone( nArea ))
				{
					PartyMemberGuids[nActualMembers] = c_PlayerGetPartyMemberGUID(nMember);
					nPartyLevelAreas[nActualMembers] = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
					nActualMembers++;
								
				}
			}
		}

		nMember++;

	}

	int nCounter = 0;
	
	// populate the window with the choices
	UI_COMPONENT *pChild = pRunegates->m_pFirstChild;
	if( nLastLevelArea != INVALID_ID )
	{
		nCounter = 1;
		while (pChild)
		{
			if (pChild->m_dwParam != 0)
			{
				if ( (int)pChild->m_dwParam == 1)
				{

					pChild->m_dwParam2 = 0;//(DWORD)UnitGetId(pRunestone);
					pChild->m_dwParam3 = 0;
					
					if (pChild->m_eComponentType == UITYPE_LABEL)
					{
						int nLevelAreaID = nLastLevelArea;
						WCHAR NameString[ 1024 ];
						WCHAR FinalString[ 1024 ];
						MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID, -1, &NameString[0], 1024, TRUE );
						int nColor = MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( nLevelAreaID, UIGetControlUnit() );//  FONTCOLOR_GREEN;

						PStrPrintf( FinalString, arrsize(FinalString), L"Opened Gate To " );
						
						UIColorCatString(FinalString, arrsize(FinalString), (FONTCOLOR)nColor, NameString);


						UILabelSetText( pChild, FinalString );

					}
					UIComponentSetActive( pChild, TRUE );
					if( pChild->m_Position.m_fY + pChild->m_fHeight > fY )
					{
						fY = pChild->m_Position.m_fY + pChild->m_fHeight;
					}
				}
				else
				{
					UIComponentSetVisible( pChild, FALSE );
				}

			}
			pChild = pChild->m_pNextSibling;
		}
	}

	pChild = pRunegates->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_dwParam != 0)
		{
			if ( (int)pChild->m_dwParam - nCounter <= nActualMembers && (int)pChild->m_dwParam > nCounter )
			{
				int nMember = (int)pChild->m_dwParam - 1 - nCounter;
				if( nMember < nActualMembers )
				{
					pChild->m_dwParam2 = 0;//(DWORD)UnitGetId(pRunestone);
					pChild->m_dwParam3 = pChild->m_dwParam - nCounter;
					if (pChild->m_eComponentType == UITYPE_LABEL)
					{
						int nLevelAreaID = nPartyLevelAreas[nMember];
						WCHAR NameString[ 1024 ];
						WCHAR FinalString[ 1024 ];
						MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID, -1, &NameString[0], 1024, TRUE );
						int nMinDiff = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID, UIGetControlUnit() );
						int nMaxDiff = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID, UIGetControlUnit() );
						int nColor = MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( nLevelAreaID, UIGetControlUnit() );//  FONTCOLOR_GREEN;

						UISetSimpleHoverText( g_UI.m_Cursor, c_PlayerGetPartyMemberName(nMember), TRUE );		
						if( nMinDiff == nMaxDiff )
						{
							PStrPrintf( FinalString, arrsize(FinalString), L"%s(Level : %d) ", c_PlayerGetPartyMemberName(nMember), nMaxDiff );
						}
						else 
						{
							PStrPrintf( FinalString, arrsize(FinalString), L"%s(Level : %d-%d) ", c_PlayerGetPartyMemberName(nMember), nMinDiff, nMaxDiff );
						}
						UIColorCatString(FinalString, arrsize(FinalString), (FONTCOLOR)nColor, NameString);


						UILabelSetText( pChild, FinalString );

					}
					if( pChild->m_Position.m_fY + pChild->m_fHeight > fY )
					{
						fY = pChild->m_Position.m_fY + pChild->m_fHeight;
					}
					UIComponentSetActive( pChild, TRUE );

				}
				else if( (int)pChild->m_dwParam > nCounter )
				{
					UIComponentSetVisible( pChild, FALSE );
				}
				
			}
			else if( (int)pChild->m_dwParam > nCounter )
			{
				UIComponentSetVisible( pChild, FALSE );
			}

		}
		pChild = pChild->m_pNextSibling;
	}
	nCounter += nActualMembers;
	pChild = pRunegates->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_dwParam != 0)
		{
			if ( (int)pChild->m_dwParam - nCounter <= nKnownRunegates && (int)pChild->m_dwParam > nCounter )
			{
				UNIT* pRunestone = pKnownRunegateAreas[pChild->m_dwParam - 1 - nCounter];
				pChild->m_dwParam2 = (DWORD)UnitGetId(pRunestone);
				pChild->m_dwParam3 = 0;
				if (pChild->m_eComponentType == UITYPE_LABEL)
				{
					int nLevelAreaID = MYTHOS_MAPS::GetLevelAreaID( pRunestone );
					WCHAR NameString[ 1024 ];
					WCHAR FinalString[ 1024 ];
					MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID, -1, &NameString[0], 1024, TRUE );
					int nMinDiff = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID, UIGetControlUnit() );
					int nMaxDiff = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID, UIGetControlUnit() );
					int nColor = MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( nLevelAreaID, UIGetControlUnit() );//  FONTCOLOR_GREEN;

					if( nMinDiff == nMaxDiff )
					{
						PStrPrintf( FinalString, arrsize(FinalString), L"(Level : %d) ", nMaxDiff );
					}
					else 
					{
						PStrPrintf( FinalString, arrsize(FinalString), L"(Level : %d-%d) ", nMinDiff, nMaxDiff );
					}
					UIColorCatString(FinalString, arrsize(FinalString), (FONTCOLOR)nColor, NameString);


					UILabelSetText( pChild, FinalString );

				}
				UIComponentSetActive( pChild, TRUE );
				if( pChild->m_Position.m_fY + pChild->m_fHeight > fY )
				{
					fY = pChild->m_Position.m_fY + pChild->m_fHeight;
				}
			}
			else if( (int)pChild->m_dwParam > nCounter )
			{
				UIComponentSetVisible( pChild, FALSE );
			}

		}
		pChild = pChild->m_pNextSibling;
	}


	if (pScrollbar)
	{	
	
		pScrollbar->m_fMin = 0;
		pScrollbar->m_fMax = fY - pRunegates->m_fHeight;
		if (pScrollbar->m_fMax < 0.0f)
		{
			pScrollbar->m_fMax = 0.0f;
			UIComponentSetVisible(pScrollbar, FALSE);
		}
		else
		{
			UIComponentSetActive(pScrollbar, TRUE);
		}
		pRunegates->m_fScrollVertMax = pScrollbar->m_fMax;
		pRunegates->m_ScrollPos.m_fY = MIN(pRunegates->m_ScrollPos.m_fY, pRunegates->m_fScrollVertMax);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowRunegateScreen()
{
	UI_COMPONENT * component = UIComponentGetByEnum(UICOMP_RUNEGATE_PANEL);
	if (!component)
	{
		return;
	}
	UIStopSkills();
	c_PlayerClearAllMovement(AppGetCltGame());
	UIComponentActivate(component, TRUE);
	UI_TB_HideWorldMapScreen( TRUE );
	sUpdateRunegateButtons(component->m_pFirstChild );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRunegateButtonDown(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIButtonOnButtonDown(component, msg, wParam, lParam);
	if (!ResultIsHandled(eResult))
	{
		return eResult;
	}

	int nIndex = (int)component->m_dwParam2;	
	REF( nIndex );
	int nMember = (int)component->m_dwParam3;	
	REF( nMember );

	MSG_CCMD_OPERATE_RUNEGATE runemsg;
	runemsg.idRunestone = nIndex;
	runemsg.guidPartyMember = nMember != 0 ? ( c_PlayerGetPartyMemberGUID(nMember - 1 ) ) : INVALID_GUID;
	runemsg.nLevelArea = ( nIndex == 0 && nMember == 0 ) ? 1 : INVALID_LINK;
	runemsg.idGameOfPartyMember = nMember != 0 ?c_PlayerGetPartyMemberGameId(nMember - 1 ) : INVALID_ID; 
//	runemsg.nAction = 1;
	c_SendMessage(CCMD_OPERATE_RUNEGATE, &runemsg);

	UI_COMPONENT * pPanel = UIComponentGetByEnum(UICOMP_RUNEGATE_PANEL);
	if (!pPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentActivate(pPanel, FALSE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRunegateCancelButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIButtonOnButtonDown(component, msg, wParam, lParam);
	if (!ResultIsHandled(eResult))
	{
		return eResult;
	}
	UI_COMPONENT * pPanel = UIComponentGetByEnum(UICOMP_RUNEGATE_PANEL);
	if (!pPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentActivate(pPanel, FALSE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadInvGrid(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_INVGRID* invgrid = UICastToInvGrid(component);

	XML_LOADEXCELILINK("invloc", invgrid->m_nInvLocDefault, DATATABLE_INVLOCIDX);
	invgrid->m_nInvLocation = invgrid->m_nInvLocDefault;
	
	XML_LOADEXCELILINK("invloccabalist", invgrid->m_nInvLocCabalist, DATATABLE_INVLOCIDX);
	XML_LOADEXCELILINK("invlochunter", invgrid->m_nInvLocHunter, DATATABLE_INVLOCIDX);
	XML_LOADEXCELILINK("invloctemplar", invgrid->m_nInvLocTemplar, DATATABLE_INVLOCIDX);
	XML_LOADEXCELILINK("invlocmerchant", invgrid->m_nInvLocMerchant, DATATABLE_INVLOCIDX);
	XML_LOADEXCELILINK("invlocplayer", invgrid->m_nInvLocPlayer, DATATABLE_INVLOCIDX);

	UIXMLLoadColor(xml, component, "mod_wont_fit_", invgrid->m_dwModWontFitColor, 0x9e200000);
	UIXMLLoadColor(xml, component, "mod_wont_go_in_item_", invgrid->m_dwModWontGoInItemColor, 0x9e202020);
	UIXMLLoadColor(xml, component, "inactive_item_", invgrid->m_dwInactiveItemColor, 0x9e404040);

	XML_LOADFLOAT("cellx", invgrid->m_fCellWidth, 0.0f);
	XML_LOADFLOAT("celly", invgrid->m_fCellHeight, 0.0f);
	XML_LOADFLOAT("cellborder", invgrid->m_fCellBorder, 0.0f);

	invgrid->m_fCellWidth *= tUIXml.m_fXmlWidthRatio;
	invgrid->m_fCellHeight *= tUIXml.m_fXmlHeightRatio;
	invgrid->m_fCellBorder *= tUIXml.m_fXmlWidthRatio;

	UIX_TEXTURE* texture = UIComponentGetTexture(invgrid);
	if (texture)
	{
		XML_LOADFRAME("litframe",			texture, invgrid->m_pItemBkgrdFrame, "");
		XML_LOADFRAME("useableborder",		texture, invgrid->m_pUseableBorderFrame, "");
		XML_LOADFRAME("socketframe",		texture, invgrid->m_pSocketFrame, "");
		XML_LOADFRAME("modframe",			texture, invgrid->m_pModFrame, "");
	}

	UI_COMPONENT *pIconPanel = UIComponentFindChildByName(component, "icon panel");
	UIX_TEXTURE *pIconTexture = (pIconPanel ? UIComponentGetTexture(pIconPanel) : NULL);
	if (pIconTexture)
	{
		XML_LOADFRAME("unidentifiedicon",		pIconTexture, invgrid->m_pUnidentifiedIconFrame, "");
		XML_LOADFRAME("badclassicon",			pIconTexture, invgrid->m_pBadClassIconFrame, "");
		XML_LOADFRAME("badclassuicon",			pIconTexture, invgrid->m_pBadClassUnIDIconFrame, "");
		XML_LOADFRAME("lowstatsicon",			pIconTexture, invgrid->m_pLowStatsIconFrame, "");
		XML_LOADFRAME("recipecompleteicon",		pIconTexture, invgrid->m_pRecipeCompleteFrame, "");
		XML_LOADFRAME("recipeincompleteicon",	pIconTexture, invgrid->m_pRecipeIncompleteFrame, "");
	}

	invgrid->m_nGridWidth = (int)((invgrid->m_fWidth / (invgrid->m_fCellWidth + invgrid->m_fCellBorder)) + 0.5f);
	invgrid->m_nGridHeight = (int)((invgrid->m_fHeight / (invgrid->m_fCellHeight + invgrid->m_fCellBorder)) + 0.5f);
	invgrid->m_pItemAdj = (UI_INV_ITEM_GFX_ADJUSTMENT *)MALLOCZ(NULL, sizeof(UI_INV_ITEM_GFX_ADJUSTMENT) * invgrid->m_nGridWidth * invgrid->m_nGridHeight);
	invgrid->m_pItemAdjStart = (UI_INV_ITEM_GFX_ADJUSTMENT *)MALLOCZ(NULL, sizeof(UI_INV_ITEM_GFX_ADJUSTMENT) * invgrid->m_nGridWidth * invgrid->m_nGridHeight);
	for (int i=0; i < (invgrid->m_nGridWidth * invgrid->m_nGridHeight); i++)
	{
		invgrid->m_pItemAdj[i].fScale = 1.0f;
	}
	invgrid->m_tItemAdjAnimInfo.m_dwAnimTicket = INVALID_ID;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIInvGridFree(
	UI_COMPONENT* component)
{
	UI_INVGRID* invgrid = UICastToInvGrid(component);
	ASSERT_RETURN(invgrid);

	FREE(NULL, invgrid->m_pItemAdj);
	FREE(NULL, invgrid->m_pItemAdjStart);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITradesmanSheetFree(
	UI_COMPONENT* component)
{

}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingPageOnPaint(
								 UI_COMPONENT* component,
								 int msg,
								 DWORD wParam,
								 DWORD lParam)
{
	UIComponentRemoveAllElements(component);

	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (!pFocusUnit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (msg == UIMSG_SETCONTROLSTAT)
	{
		UI_COMPONENT* pTooltip = UIComponentGetByEnum(UICOMP_SKILLTOOLTIP);
		if (pTooltip)
		{
			int nSkill = (int)pTooltip->m_dwData;
			pTooltip->m_dwData = (DWORD) INVALID_LINK;		// reset the cached tooltip skill
			if (UIComponentGetActive(pTooltip))
			{
				UISetHoverTextSkill(component, UIGetControlUnit(), nSkill, NULL, FALSE, TRUE);
			}
		}
	}

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	const UNIT_DATA *pUnitData = UnitGetData(pFocusUnit);
	ASSERT_RETVAL(pUnitData, UIMSG_RET_NOT_HANDLED);

	UIComponentAddFrame(component);

	// Label the tab buttons	(may need to move this to some sort of post-load process)
	for ( UI_COMPONENT *pChild = component->m_pFirstChild; pChild; pChild = pChild->m_pNextSibling )
	{
		if (msg != UIMSG_PAINT)
		{
			// paint the children
			UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
		}

		if (pChild->m_eComponentType != UITYPE_BUTTON)
			continue;

		UI_BUTTONEX *pButton = UICastToButton(pChild);

		int nSkillTab = sGetSkillTab( pFocusUnit, pButton->m_nAssocTab );


		if ( nSkillTab != INVALID_LINK )
		{
			SKILLTAB_DATA * pSkilltabData = (SKILLTAB_DATA *)ExcelGetData(pGame, DATATABLE_SKILLTABS, nSkillTab );
			UIComponentSetVisible(pChild, TRUE);
			if (pChild->m_pFirstChild && 
				pChild->m_pFirstChild->m_eComponentType == UITYPE_LABEL)
			{
				UIComponentSetActive(pChild, TRUE);
#if ISVERSION(DEVELOPMENT)
				const char *pszKey = StringTableGetKeyByStringIndex( pSkilltabData->nDisplayString );
				UILabelSetTextByStringKey( pChild->m_pFirstChild, pszKey );
#else
				UILabelSetText(pChild->m_pFirstChild, StringTableGetStringByIndex(pSkilltabData->nDisplayString));
#endif

				if( AppIsTugboat() )
				{
					UI_PANELEX* pPanel = UICastToPanel( pButton->m_pParent );

					if( pPanel->m_nCurrentTab == pButton->m_nAssocTab )
					{
						UI_LABELEX * pLabel = UICastToLabel( UIComponentFindChildByName( pPanel, "skill pane header" ) );
						UILabelSetText(pLabel, StringTableGetStringByIndex(pSkilltabData->nDisplayString));
						if( pLabel->m_pParent->m_eComponentType == UITYPE_TOOLTIP )
						{
							UITooltipDetermineContents(pLabel->m_pParent);
							UITooltipDetermineSize(pLabel->m_pParent);
						}
					}


				}

			}
		}
		else
		{
			UIComponentSetVisible(pChild, FALSE);
		}
	}


	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnSetControlUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);

	// The control unit has changed, so we need to re-look up the tab numbers as the class may have changed.
	pGrid->m_bLookupTabNum = TRUE;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline UI_POSITION sGetCraftingSkillPosOnPage(
	const SKILL_DATA *pSkillData,
	const UI_CRAFTINGGRID *pGrid,
	int nCol,
	int nRow )
{
	UI_POSITION pos;
	pos.m_fX = nCol * (pGrid->m_fCellWidth + pGrid->m_fCellBorderX) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f);
	pos.m_fY = nRow * (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f);
	return pos;
}

static inline BOOL sCraftingSkillVisibleOnPage(
									   int nSkill,
									   const SKILL_DATA *pSkillData,
									   const UI_CRAFTINGGRID *pGrid,
									   UNIT *pUnit,
									   BOOL bDrawOnlyKnown)
{
	if (!(pSkillData &&
		pSkillData->nSkillTab == pGrid->m_nSkillTab &&
		pSkillData->nSkillPageX > INVALID_LINK &&
		pSkillData->nSkillPageY > INVALID_LINK &&
		pSkillData->nSkillPageX < pGrid->m_nCellsX &&
		pSkillData->nSkillPageY < pGrid->m_nCellsY))
	{
		return FALSE;
	}

	BOOL bHasSkill = (SkillGetLevel(pUnit, nSkill) > 0);
	if (!bHasSkill && bDrawOnlyKnown)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


#define TIER_DEPTH 5
#define TIERS_HIGH 10


//////////////////////////////////////////////////////////////////////////////
// set focus unit for crafting tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UICraftingPointAddButtonSetVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT* pUnit = UIComponentGetFocusUnit(component);

	BOOL bVisible = FALSE;
	if (pUnit)
	{
		GAME *pGame = UnitGetGame(pUnit);
		REF( pGame );
		if ((UnitIsPlayer(pUnit) || (AppIsHellgate() && UnitGetGame(pUnit) && PetIsPlayerPet(UnitGetGame(pUnit), pUnit)))&& UnitGetStat(pUnit, STATS_CRAFTING_POINTS) > 0)
		{
			bVisible = TRUE;
		}
		if( AppIsTugboat() &&
			UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, (int)component->m_dwParam ) >= 10 )
		{
			bVisible = FALSE;
		}
	}


	UIComponentSetActive(component, bVisible);
	UIComponentSetVisible(component, bVisible, TRUE);
	return UIMSG_RET_HANDLED;
} // UICraftingPointAddButtonSetVisible()


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{	
	if (msg == UIMSG_SETCONTROLSTAT && !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);

	UIComponentRemoveAllElements(pGrid);
	UIComponentRemoveAllElements(pGrid->m_pIconPanel);
	UIComponentRemoveAllElements(pGrid->m_pHighlightPanel);

	if (pGrid->m_nSkillTab > INVALID_LINK || pGrid->m_bLookupTabNum)
	{
		GAME *pGame = AppGetCltGame();
		if (!pGame)
			return UIMSG_RET_NOT_HANDLED;
		UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
		if (!pFocusUnit)
			return UIMSG_RET_NOT_HANDLED;
		UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
		if (!pTexture)		
			return UIMSG_RET_NOT_HANDLED;

		if ( pGrid->m_bLookupTabNum )
		{
			pGrid->m_nSkillTab = sGetSkillTab( pFocusUnit, (int)pGrid->m_dwData );
			if ( pGrid->m_nSkillTab != INVALID_ID )
				pGrid->m_bLookupTabNum = FALSE;
		}
		UI_COMPONENT* pPlusButton = UIComponentGetByEnum(UICOMP_CRAFTING_TIER_PLUS_BUTTON);
		switch( pGrid->m_dwData )
		{
		case 5 :
			pPlusButton = UIComponentGetByEnum(UICOMP_CRAFTING_TIER_PLUS_BUTTON2);
			break;
		case 6 :
			pPlusButton = UIComponentGetByEnum(UICOMP_CRAFTING_TIER_PLUS_BUTTON3);
			break;
		}
		
		if( pPlusButton )
		{
			pPlusButton->m_dwParam = pGrid->m_nSkillTab;	
			UICraftingPointAddButtonSetVisible( pPlusButton, 0, 0, 0 );
		}


		const SKILLTAB_DATA *pSkillTabData = pGrid->m_nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pGrid->m_nSkillTab) : NULL;

		UI_COMPONENT* pTierBar = component->m_pParent;
		pTierBar = UIComponentFindChildByName( pTierBar, "skill_tier_bar_vert");


		UI_COMPONENT* pGrayBarPool = component->m_pParent;
		UI_COMPONENT* pCurrentGrayBar = NULL;
		if( pGrayBarPool )
		{
			pGrayBarPool = UIComponentFindChildByName( pGrayBarPool, "skill gray panels");
			ASSERTX_RETVAL( pGrayBarPool, UIMSG_RET_HANDLED, "no gray bars were found for the skill pane" ); 
			pCurrentGrayBar = pGrayBarPool->m_pFirstChild;
		}

		UI_COMPONENT* pBarPool = component->m_pParent;
		UI_COMPONENT* pCurrentBar = NULL;
		if( pBarPool )
		{
			pBarPool = UIComponentFindChildByName( pBarPool, "skill bar panel");
			ASSERTX_RETVAL( pBarPool, UIMSG_RET_HANDLED, "no display bars were found for the skill pane" ); 
			pCurrentBar = pBarPool->m_pFirstChild;
		}

		UI_COMPONENT* pBarLargePool = component->m_pParent;
		UI_COMPONENT* pCurrentBarLarge = NULL;
		if( pBarLargePool )
		{
			pBarLargePool = UIComponentFindChildByName( pBarLargePool, "skill bar panel large");
			ASSERTX_RETVAL( pBarLargePool, UIMSG_RET_HANDLED, "no display BarLarges were found for the skill pane" ); 
			pCurrentBarLarge = pBarLargePool->m_pFirstChild;
		}

		UI_COMPONENT* pArrowPool = component->m_pParent;
		UI_COMPONENT* pCurrentArrow = NULL;
		if( pArrowPool )
		{
			pArrowPool = UIComponentFindChildByName( pArrowPool, "skill arrow panel");
			ASSERTX_RETVAL( pArrowPool, UIMSG_RET_HANDLED, "no arrows were found for the skill pane" ); 
			pCurrentArrow = pArrowPool->m_pFirstChild;
		}

		UI_COMPONENT* pConnecterPool = component->m_pParent;
		UI_COMPONENT* pCurrentConnecter = NULL;
		if( pConnecterPool )
		{
			pConnecterPool = UIComponentFindChildByName( pConnecterPool, "skill line panel");
			ASSERTX_RETVAL( pConnecterPool, UIMSG_RET_HANDLED, "no lines were found for the skill pane" ); 
			pCurrentConnecter = pConnecterPool->m_pFirstChild;
		}

		UI_COMPONENT* pLinePool = component->m_pParent;
		UI_COMPONENT* pCurrentLine = NULL;
		if( pLinePool )
		{
			pLinePool = UIComponentFindChildByName( pLinePool, "skill divider panel");
			ASSERTX_RETVAL( pLinePool, UIMSG_RET_HANDLED, "no arrows were found for the skill pane" ); 
			pCurrentLine = pLinePool->m_pFirstChild;
		}

		int nGridWidth = pGrid->m_nCellsX;
		int nGridHeight = pGrid->m_nCellsY;

		int  skillGridSkillIDs[10][10];
		int  skillGridSkillPoints[10][10];
		int  skillGridSkillPointsInvested[10][10];
		int  skillGridBarHeight[10][10];

		for( int i = 0; i < nGridWidth; i++ )
		{
			for( int j = 0; j < nGridHeight; j++ )
			{
				skillGridSkillIDs[i][j] = INVALID_ID;
				skillGridSkillPoints[i][j] = 0;
				skillGridSkillPointsInvested[i][j] = 0;
				skillGridBarHeight[i][j] = 0;
			}
		}

		int nSkillCount = SkillsGetCount(pGame);
		int nTotalSkillPointsInvested = UnitGetStat(pFocusUnit, STATS_SKILL_POINTS_TIER_SPENT, pGrid->m_nSkillTab );


		//first we find all the skills we need on the page and where on the page they belog. As
		//well as how many points each skill can have invested.
		for (int iSkillIndex = 0; iSkillIndex < nSkillCount; iSkillIndex++)
		{
			const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iSkillIndex);

			if (pSkillData &&
				sCraftingSkillVisibleOnPage( iSkillIndex, pSkillData, pGrid, pFocusUnit, FALSE ))
			{		

				int nInitialLevel = SkillGetSkillLevelValue( pSkillData->pnSkillLevelToMinLevel[0] - 1, KSkillLevel_CraftingTier );
				int nCol = pSkillData->nSkillPageX;
				int nRow = nInitialLevel;
				for( int nLevelIndex = 0; nLevelIndex < MAX_LEVELS_PER_SKILL; nLevelIndex++ ) 
				{					
					if( pSkillData->pnSkillLevelToMinLevel[nLevelIndex] > 0 )
					{
						if( skillGridSkillIDs[ nCol ][ nRow ] == INVALID_ID || 
							skillGridSkillIDs[ nCol ][ nRow ] == iSkillIndex )
						{
							if( skillGridSkillIDs[ nCol ][ nRow ] == INVALID_ID )
							{
								skillGridBarHeight[ nCol ][ nRow ] = 1;
								for( int k = nRow - 1 ; k >= 0; k-- )
								{
									// check if there was a previous bar on this chain, and calculate its
									// size based on how far it is away from this one
									if( skillGridSkillIDs[ nCol ][ k ] != INVALID_ID )
									{
										skillGridBarHeight[ nCol ][ k ] = nRow - k;
										break;
									}
								}
							}
							skillGridSkillIDs[ nCol ][nRow] = iSkillIndex;
							skillGridSkillPoints[ nCol ][nRow]++;
							if( skillGridSkillPointsInvested[ nCol ][nRow] == 0 &&
								nRow != nInitialLevel )//first row is always available
							{
								skillGridSkillPointsInvested[ nCol ][nRow] = nLevelIndex;
							}
						}
					}
				}
			}
		}

		//now lets draw the skills
		int nColAllowedToInvestIn = SkillGetSkillLevelValue( nTotalSkillPointsInvested, KSkillLevel_Tier );
		BOOL bDrawSkillupButtons = (UnitGetStat(pFocusUnit, STATS_CRAFTING_POINTS) > 0);

		//figure out where the mouse is.
		float x = 0.0f; 
		float y = 0.0f;
		UIGetCursorPosition(&x, &y);

		UI_POSITION pos;
		UIComponentCheckBounds(component, x, y, &pos);
		x -= pos.m_fX;
		y -= pos.m_fY;
		//get the icon where to draw the text onto...

		// we need logical coords for the element comparisons ( getskillatpos )
		float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
		float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);

		//First setup the skill icon draw info		
		UI_DRAWSKILLICON_PARAMS tParams(pGrid->m_pIconPanel, UI_RECT(), UI_POSITION(), -1, pGrid->m_pBkgndFrame , pGrid->m_pBorderFrame, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat(), FALSE, TRUE, 1.0f, 1.0f, pGrid->m_fIconWidth, pGrid->m_fIconHeight );
		tParams.pBackgroundComponent = pGrid;
		if (pSkillTabData)
		{
			//for the texture in the background - *i think*
			tParams.pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
		}

		// gray out tiers we don't have access to.
		int nTierCounter = 0;

		int nPreviousTier = -1;
		while( nTierCounter <= nTotalSkillPointsInvested && pCurrentGrayBar )
		{
			if( SkillGetSkillLevelValue( nTierCounter, KSkillLevel_CraftingTier ) != nPreviousTier  )
			{
				nPreviousTier = SkillGetSkillLevelValue( nTierCounter, KSkillLevel_CraftingTier );
				UIComponentSetVisible( pCurrentGrayBar, FALSE );
				UIComponentHandleUIMessage(pCurrentGrayBar, UIMSG_PAINT, 0, 0);

				pCurrentGrayBar = pCurrentGrayBar->m_pNextSibling;
			}
			nTierCounter++;

		}
		while( pCurrentGrayBar )
		{
			UIComponentSetActive( pCurrentGrayBar, TRUE );
			UIComponentHandleUIMessage(pCurrentGrayBar, UIMSG_PAINT, 0, 0);
			pCurrentGrayBar = pCurrentGrayBar->m_pNextSibling;
		}


		// fill the Tier bar
		UI_BAR* pBar = UICastToBar( UIComponentFindChildByName(pTierBar, "skill bar") );

		int nTiers = 5;
		int nMaxPoints = 10;
		int nTier = SkillGetSkillLevelValue( nTotalSkillPointsInvested, KSkillLevel_CraftingTier );
		float fValue = ( (float)nTier / (float)nTiers ) * 100;
		if( nTotalSkillPointsInvested == nMaxPoints )
		{
			fValue = 100;
		}
		else
		{
			float fNextValue = ( (float)min( nTier + 1, nTiers ) / (float)nTiers ) * 100;
			for( int i = 0; i <= nMaxPoints; i++ )
			{
				if( SkillGetSkillLevelValue( i, KSkillLevel_CraftingTier ) == nTier )
				{

					for( int j = i; j <= nMaxPoints; j++ )
					{
						if( SkillGetSkillLevelValue( j, KSkillLevel_CraftingTier ) != nTier ||
							j == nMaxPoints )
						{
							fValue += ( (float)( nTotalSkillPointsInvested - i) / (float)( j - i ) ) * ( fNextValue - fValue );
							break;
						}
					}
					break;
				}
			}
		}

		pBar->m_nValue = (int)fValue;
		pBar->m_nMaxValue = 100;//SKILL_POINTS_PER_TIER_TO_UNLOCK * 10; need to get this from data

		UIComponentHandleUIMessage(pTierBar, UIMSG_PAINT, 0, 0);
		// Display a count of the current point invenstment in this pane,
		// and the next tier goal
		if( UIComponentGetVisible( component ) )
		{
			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			if (!pFont)
			{
				pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
			}
			//position
			UI_POSITION textpos(pTierBar->m_Position.m_fX + pTierBar->m_fWidth / 2, pTierBar->m_Position.m_fY  + 370 * g_UI.m_fUIScaler );
			int nFontSize = UIComponentGetFontSize(component);
			UI_SIZE size((pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : ( pGrid->m_pBkgndFrame->m_fWidth - pGrid->m_pBkgndFrame->m_fXOffset )), (float)nFontSize);
			textpos.m_fX -= size.m_fWidth / 2;
			textpos.m_fX -= pGrid->m_Position.m_fX;
			//color
			DWORD dwColor = UI_MAKE_COLOR(255, GFXCOLOR_WHITE);	   
			WCHAR szPoints[256];	
			nFontSize -= 4;
			PStrPrintf(szPoints, arrsize(szPoints), L"%d/%d", min( nMaxPoints, nTotalSkillPointsInvested ), nMaxPoints );
			UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );

			int nCurrentTier = 0;
			textpos.m_fX -= 20 * g_UI.m_fUIScaler;

			for( int i = 0; i <= nMaxPoints; i++ )
			{

				if( SkillGetSkillLevelValue( i, KSkillLevel_CraftingTier ) > nCurrentTier )
				{
					float Offset = (float)( nCurrentTier + 1 ) / (float) nTiers * 350 - 10;

					textpos.m_fY = pTierBar->m_Position.m_fY  + Offset * g_UI.m_fUIScaler;
					PStrPrintf(szPoints, arrsize(szPoints), L"%d", i );
					UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );

					nCurrentTier++;							
				}
			}
		}


		for( int nCol = 0; nCol < nGridWidth; nCol++ )
		{	
			int nShownSkillID = INVALID_ID;
			for( int nRow = 0; nRow < nGridHeight; nRow++ )
			{


				//ASSERTX_RETVAL( nIndexIntoGrid < SKILL_PANEL_MAX_COUNT, UIMSG_RET_HANDLED, "Error in getting index for skills. Skill index is out of range." ); 
				int nSkillIndex = skillGridSkillIDs[ nCol ][ nRow ];
				if( nSkillIndex > 0 )
				{
					BOOL bSkillDisplayed = FALSE;
					const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, nSkillIndex );
					ASSERTX_RETVAL( pSkillData, UIMSG_RET_HANDLED, "Skill retrieved from Skill grid id's is invalid." ); 
					int nSkillPointsInvested = SkillGetLevel(pFocusUnit, nSkillIndex);
					//if the skills accessible
					int nSkillPointsInvestOnLevel = max( 0, nSkillPointsInvested - skillGridSkillPointsInvested[ nCol ][ nRow ]);
					DWORD nFlags = SkillGetNextLevelFlags( pFocusUnit, nSkillIndex );
					BOOL bSkillAccessible = ( ( SkillGetSkillLevelValue( nTotalSkillPointsInvested, KSkillLevel_CraftingTier ) + 1 ) > (nRow) &&
						!TESTBIT( nFlags, SKILLLEARN_BIT_MISSING_PREREQUISITE_SKILL ) &&
						( skillGridSkillPointsInvested[ nCol ][ nRow ] == 0 ||
						( nRow <= nColAllowedToInvestIn &&
						nSkillPointsInvested > 0 &&
						skillGridSkillPointsInvested[ nCol ][ nRow ] <= nSkillPointsInvested )) );

					//position and size
					tParams.rect.m_fX1 = nCol * (pGrid->m_fCellWidth + pGrid->m_fCellBorderX);
					tParams.rect.m_fY1 = nRow * (pGrid->m_fCellHeight + pGrid->m_fCellBorderY);				
					ASSERTX_RETVAL(pGrid->m_pBkgndFrame || pGrid->m_pBorderFrame, UIMSG_RET_NOT_HANDLED, "Skills Grid must have a border frame or background frame to draw properly");
					tParams.rect.m_fX2 = tParams.rect.m_fX1 + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fWidth : ( pGrid->m_pBkgndFrame->m_fWidth - pGrid->m_pBkgndFrame->m_fXOffset ));
					tParams.rect.m_fY2 = tParams.rect.m_fY1 + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fHeight : ( pGrid->m_pBkgndFrame->m_fHeight - pGrid->m_pBkgndFrame->m_fYOffset ));
					tParams.pos = sGetCraftingSkillPosOnPage(pSkillData, pGrid, nCol, nRow);
					//end position and size
					//is the mouse over it?
					tParams.bMouseOver = UIInRect(UI_POSITION(x,y), tParams.rect);
					//this is the selected frame. Leaving NULL for now.
					tParams.pSelectedFrame = NULL;
					tParams.bGreyout = !bSkillAccessible;
					tParams.fSkillIconWidth = pGrid->m_fIconWidth;
					tParams.fSkillIconHeight = pGrid->m_fIconHeight;
					tParams.nSkill = nSkillIndex;
					//draw the icon
					if( pSkillData->eUsage == USAGE_PASSIVE && pGrid->m_pBkgndFramePassive )
					{
						tParams.pBackgroundFrame = pGrid->m_pBkgndFramePassive;
					}
					else
					{
						tParams.pBackgroundFrame = pGrid->m_pBkgndFrame;
					}

					// if a skill was displayed, we need to know to adjust the bar height;
					if( nShownSkillID != nSkillIndex )
					{
						UIDrawSkillIcon(tParams);
						bSkillDisplayed = TRUE;
					}

					BOOL bFirst = TRUE;

					if( pCurrentArrow && pCurrentLine && UIComponentGetVisible( component ) )
					{
						for( int k = nRow - 1 ; k >= 0; k-- )
						{
							// check if there was a previous bar on this chain - if so, we don't do this at all
							// size based on how far it is away from this one
							if( skillGridSkillIDs[ nCol ][ k ] == skillGridSkillIDs[ nCol ][ nRow ] )
							{
								bFirst = FALSE;
								break;
							}
						}
						if( bFirst )
						{

							// let's draw arrows from any parents we depend on 
							for( int nSkillReq = 0; nSkillReq < MAX_PREREQUISITE_SKILLS; nSkillReq++ )
							{
								int nSkillToReq = pSkillData->pnRequiredSkills[ nSkillReq ];
								int nPointsToInvest = max( 1, pSkillData->pnRequiredSkillsLevels[ nSkillReq ] ) - 1;
								if( nSkillToReq != INVALID_ID )
								{						
									const SKILL_DATA* pSkillDataPreReq = SkillGetData( NULL, nSkillToReq);
									ASSERT( pSkillDataPreReq );	
									int nSkillLvl = pSkillDataPreReq->pnSkillLevelToMinLevel[ nPointsToInvest ]; //get the skill
									int nReqRow = nSkillLvl/SKILL_POINTS_PER_TIER; //find the row in the current grid
									int nReqCol = pSkillDataPreReq->nSkillPageX;//get the col
									//ok lets draw the lines.....
									//first position of the skill required
									UI_POSITION skillPosReq = sGetCraftingSkillPosOnPage(pSkillDataPreReq, pGrid, nReqCol, nReqRow);
									skillPosReq.m_fX += pGrid->m_fIconWidth/2.0f;
									skillPosReq.m_fX *= UIGetScreenToLogRatioX( pGrid->m_bNoScale); //center point
									skillPosReq.m_fY += pGrid->m_fIconHeight/2.0f;
									skillPosReq.m_fY *= UIGetScreenToLogRatioY( pGrid->m_bNoScale); //center point


									float ReqX = nReqCol * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (tParams.rect.m_fX2 - tParams.rect.m_fX1 ) / 2;
									float ReqY = nReqRow * ( (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f) ) ;	
									float X = nCol * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (tParams.rect.m_fX2 - tParams.rect.m_fX1 ) / 2;
									float Y = nRow * ( (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f) ) ;	

									float Height = Y - ReqY; 


									if( pCurrentArrow && nReqCol != nCol )
									{

										pCurrentArrow->m_Position.m_fX = X - pCurrentArrow->m_fWidth / 2;
										pCurrentArrow->m_Position.m_fY = ReqY;
										pCurrentArrow->m_fHeight = Height;
										UIComponentSetActive( pCurrentArrow, TRUE );						
										UIComponentHandleUIMessage(pCurrentArrow, UIMSG_PAINT, 0, 0);

										pCurrentArrow = pCurrentArrow->m_pNextSibling;


										pCurrentLine->m_Position.m_fY = ReqY;
										pCurrentLine->m_fWidth = ( abs(nCol - nReqCol) - .5f ) * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (tParams.rect.m_fX2 - tParams.rect.m_fX1 ) / 2;
										// right side 
										if( nCol > nReqCol )
										{
											pCurrentLine->m_Position.m_fX = ReqX  + 5;
										}
										else
										{
											pCurrentLine->m_Position.m_fX = ReqX - pCurrentLine->m_fWidth -  5;
										}
										UIComponentSetActive( pCurrentLine, TRUE );						
										UIComponentHandleUIMessage(pCurrentLine, UIMSG_PAINT, 0, 0);

										pCurrentLine = pCurrentLine->m_pNextSibling;
									}

								}

							}
						}
					}

					// let's show a progress bar for this thing!
					if( pCurrentBar && pCurrentBarLarge && UIComponentGetVisible(component))
					{
						BOOL bLargeIcon = FALSE;
						if( bSkillAccessible && min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]) != skillGridSkillPoints[ nCol ][ nRow ] )
						{
							bLargeIcon = TRUE;
						}
						UI_COMPONENT* pThisBar = bLargeIcon ? pCurrentBarLarge : pCurrentBar;
						// calculate the height and location of the bar.
						float X = nCol * ( (pGrid->m_fCellWidth + pGrid->m_fCellBorderX ) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fXOffset : 0.0f) ) + (pGrid->m_fCellWidth ) / 2 - pThisBar->m_fWidth / 2;
						float Y = ( (float)nRow + .4f ) * ( (pGrid->m_fCellHeight + pGrid->m_fCellBorderY) + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f) ) ;	
						// the following is for the 'line draw' connection method for progress bars
						float Height = pThisBar->m_fHeight;
						// adjust the height and position if a skill icon was shown on this tier

						Y += 2;
						Height -= 8;
						// adjust the size and fill of the bar
						if( bSkillAccessible )
						{

							if( pThisBar->m_pFirstChild )
							{
								UI_BAR* pBar = UICastToBar( pThisBar->m_pFirstChild );

								pBar->m_nValue = min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]);
								pBar->m_nMaxValue = skillGridSkillPoints[ nCol ][ nRow ];
							}



							// set the background's elements to show skill mouseovers
							pThisBar->m_dwData = nSkillIndex;

							pThisBar->m_Position.m_fX = X;
							pThisBar->m_Position.m_fY = Y;
							pThisBar->m_ActivePos = pThisBar->m_Position;
							UIComponentSetActive( pThisBar, TRUE );						
							UIComponentHandleUIMessage(pThisBar, UIMSG_PAINT, 0, 0);

						}

						// draw a line to the next bar, if required

						if( skillGridBarHeight[ nCol ][ nRow ] > 0 && pCurrentConnecter )
						{
							BOOL bOnEnd = TRUE;
							for( int k = nRow + 1 ; k < nGridHeight; k++ )
							{
								// check if there was a previous bar on this chain, and calculate its
								// size based on how far it is away from this one
								if( skillGridSkillIDs[ nCol ][ k ] != skillGridSkillIDs[ nCol ][ nRow ] &&
									skillGridSkillIDs[ nCol ][ k ] != INVALID_ID )
								{
									const SKILL_DATA* pSkillDataCheck = (SKILL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLS, skillGridSkillIDs[ nCol ][ k ]);
									if( pSkillDataCheck->pnRequiredSkills[ 0 ] != INVALID_ID )
									{
										bOnEnd = FALSE;
										break;
									}

								}
							}
							if( !bOnEnd )
							{
								pCurrentConnecter->m_Position.m_fX = X - pCurrentConnecter->m_fWidth / 2 + pThisBar->m_fWidth / 2;
								pCurrentConnecter->m_Position.m_fY = Y;// + Height;
								pCurrentConnecter->m_fHeight = ( skillGridBarHeight[ nCol ][ nRow ] ) * ( pGrid->m_fCellHeight + pGrid->m_fCellBorderY  + (pGrid->m_pBorderFrame ? pGrid->m_pBorderFrame->m_fYOffset : 0.0f)) - Height * .5f;
								UIComponentSetActive( pCurrentConnecter, TRUE );						
								UIComponentHandleUIMessage(pCurrentConnecter, UIMSG_PAINT, 0, 0);

								pCurrentConnecter = pCurrentConnecter->m_pNextSibling;

							}
						}

						// show current point investment vs Max
						// Add some tiny text to ID the skill
						if( min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]) != skillGridSkillPoints[ nCol ][ nRow ] )
						{
							UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
							if (!pFont)
							{
								pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_SKILLGRID1));
							}
							//position
							UI_POSITION textpos(tParams.rect.m_fX1, tParams.rect.m_fY1 + pGrid->m_fCellHeight );
							int nFontSize = UIComponentGetFontSize(component);
							textpos.m_fY = Y + Height / 2 - nFontSize * g_UI.m_fUIScaler + 12 * g_UI.m_fUIScaler;
							UI_SIZE size(pGrid->m_fCellWidth, (float)nFontSize);
							//color
							DWORD dwColor = UI_MAKE_COLOR(200, GFXCOLOR_WHITE);	   
							WCHAR szPoints[256];					
							if( bSkillAccessible )
							{
								PStrPrintf(szPoints, arrsize(szPoints), L"%d/%d", min( nSkillPointsInvestOnLevel, skillGridSkillPoints[ nCol ][ nRow ]) , skillGridSkillPoints[ nCol ][ nRow ] );
								UIComponentAddTextElement(pGrid->m_pIconPanel, pTexture, pFont, nFontSize - 4, szPoints, textpos, dwColor, NULL, UIALIGN_CENTER, &size );
							}
						
						}


						DWORD dwFlags = SkillGetNextLevelFlags(pFocusUnit, nSkillIndex);

						//draw the skill up icon
						if ( TESTBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL ) && 
							bDrawSkillupButtons && 
							pGrid->m_pSkillupButtonFrame &&
							bSkillAccessible &&
							nSkillPointsInvestOnLevel < skillGridSkillPoints[ nCol ][ nRow ] )
						{
							DWORD dwColor = pGrid->m_pSkillupButtonFrame->m_dwDefaultColor;
							UI_RECT rectFrame;
							UI_POSITION pos;
							pos.m_fX = X;
							pos.m_fY = Y + ( Height - pGrid->m_pSkillupButtonFrame->m_fHeight ) * g_UI.m_fUIScaler ;
							rectFrame.m_fX1 = pos.m_fX;
							rectFrame.m_fY1 = pos.m_fY;
							rectFrame.m_fX2 = rectFrame.m_fX1 + pGrid->m_pSkillupButtonFrame->m_fWidth;
							rectFrame.m_fY2 = rectFrame.m_fY1 + pGrid->m_pSkillupButtonFrame->m_fHeight;
							UI_TEXTURE_FRAME *pFrame = pGrid->m_pSkillupButtonFrame;
							if (UIInRect(UI_POSITION(logx,logy), rectFrame))
							{
								pFrame = pGrid->m_pSkillupButtonDownFrame;
							}
							UI_GFXELEMENT* pElement = UIComponentAddElement(pGrid->m_pIconPanel, pTexture, pFrame, pos, dwColor);
							if( pElement )
							{
								pElement->m_qwData = nSkillIndex;
							}
							if (pGrid->m_pSkillupBorderFrame)
								UIComponentAddElement(pGrid->m_pHighlightPanel, pTexture, pGrid->m_pSkillupBorderFrame, pos);
						}
						if( bSkillAccessible )
						{
							if( bLargeIcon )
							{
								pCurrentBarLarge = pCurrentBarLarge->m_pNextSibling;
							}
							else
							{
								pCurrentBar = pCurrentBar->m_pNextSibling;
							}
						}
					}
					nShownSkillID = nSkillIndex;


				}
			}
		}

		// now hide any remaining bars that weren't used
		while( pCurrentBar )
		{
			UIComponentSetVisible( pCurrentBar, FALSE );
			pCurrentBar = pCurrentBar->m_pNextSibling;
		}
		// now hide any remaining bars that weren't used
		while( pCurrentBarLarge )
		{
			UIComponentSetVisible( pCurrentBarLarge, FALSE );
			pCurrentBarLarge = pCurrentBarLarge->m_pNextSibling;
		}
		// now hide any remaining bars that weren't used
		while( pCurrentBarLarge )
		{
			UIComponentSetVisible( pCurrentBarLarge, FALSE );
			pCurrentBarLarge = pCurrentBarLarge->m_pNextSibling;
		}
		// now hide any remaining arrows that weren't used
		while( pCurrentArrow )
		{
			UIComponentSetVisible( pCurrentArrow, FALSE );
			pCurrentArrow = pCurrentArrow->m_pNextSibling;
		}
		// now hide any remaining connecters that weren't used
		while( pCurrentConnecter )
		{
			UIComponentSetVisible( pCurrentConnecter, FALSE );
			pCurrentConnecter = pCurrentConnecter->m_pNextSibling;
		}
		// now hide any remaining lines that weren't used
		while( pCurrentLine )
		{
			UIComponentSetVisible( pCurrentLine, FALSE );
			pCurrentLine = pCurrentLine->m_pNextSibling;
		}

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sCraftingGridGetSkillAtPosition(
								  UI_CRAFTINGGRID *pGrid,
								  float x, 
								  float y,
								  UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return 0;
	}

	UI_GFXELEMENT *pChild = pGrid->m_pIconPanel->m_pGfxElementFirst;
	while(pChild)
	{

		if ( UIElementCheckBounds(pChild, x, y ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID &&
			(pChild->m_qwData >> 16 == 0))
		{
			if ( (AppIsHellgate() ||
				pChild->m_fZDelta != 0) ||
				AppIsTugboat() )
			{
				pos.m_fX = pChild->m_Position.m_fX;
				pos.m_fY = pChild->m_Position.m_fY;

				return (int)(pChild->m_qwData & 0xFFFF) ;
			}
		}
		pChild = pChild->m_pNextElement;
	}
	return INVALID_LINK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sCraftingGridGetSkillBarAtPosition(
									 UI_COMPONENT* pComponent,
									 float x, 
									 float y,
									 UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}


	UI_COMPONENT *pChild = pComponent->m_pFirstChild;
	while(pChild)
	{

		UI_POSITION pos2 = UIComponentGetAbsoluteLogPosition(pChild);

		UI_RECT componentrect = UIComponentGetRect(pChild);

		if ( !UIComponentGetVisible( pChild ) ||
			x <	 pos2.m_fX + componentrect.m_fX1 || 
			x >= pos2.m_fX + componentrect.m_fX2 ||
			y <  pos2.m_fY + componentrect.m_fY1 ||
			y >= pos2.m_fY + componentrect.m_fY2 )
		{
			pChild = pChild->m_pNextSibling;
			continue;
		}

		if ( (pChild->m_dwData) != (DWORD)INVALID_ID )
		{

			pos.m_fX = pChild->m_Position.m_fX;
			pos.m_fY = pChild->m_Position.m_fY;

			return (int)pChild->m_dwData;
		}
		pChild = pChild->m_pNextSibling;
	}
	return INVALID_LINK;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sCraftingGridGetLevelUpAtPosition(
									UI_CRAFTINGGRID *pGrid,
									float x, 
									float y,
									UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return 0;
	}

	UI_GFXELEMENT *pChild = pGrid->m_pIconPanel->m_pGfxElementFirst;
	while(pChild)
	{

		if (UIElementCheckBounds(pChild, x, y ) && (pChild->m_qwData)  != (QWORD)INVALID_ID &&
			pChild->m_fZDelta == 0 )
		{
			pos.m_fX = pChild->m_Position.m_fX;
			pos.m_fY = pChild->m_Position.m_fY;
			return (int)(pChild->m_qwData & 0xFFFF) ;
		}
		pChild = pChild->m_pNextElement;
	}
	return INVALID_LINK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sCraftingGridGetSkillLevelUpAtPosition(
	UI_CRAFTINGGRID *pGrid,
	float x, 
	float y)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pGrid);
	if (!pFocusUnit)
	{
		return INVALID_LINK;
	}

	if (!pGrid->m_pSkillupButtonFrame)
	{
		return INVALID_LINK;
	}

	if (pGrid->m_nSkillTab > INVALID_LINK)
	{
		BOOL bDrawOnlyKnown = FALSE;
		const SKILLTAB_DATA *pSkillTabData = pGrid->m_nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pGrid->m_nSkillTab) : NULL;
		if (pSkillTabData)
		{
			bDrawOnlyKnown = pSkillTabData->bDrawOnlyKnown;
		}

		int nRowCount = SkillsGetCount(pGame);
		for (int iRow = 0; iRow < nRowCount; iRow++)
		{
			const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
			if (sCraftingSkillVisibleOnPage(iRow, pSkillData, pGrid, pFocusUnit, bDrawOnlyKnown))
			{
				DWORD dwFlags = SkillGetNextLevelFlags(pFocusUnit, iRow);
				BOOL bCanLevel = TESTBIT( dwFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL );
				if (bCanLevel)
				{
					UI_POSITION pos = sGetCraftingSkillPosOnPage(pSkillData, pGrid, pSkillData->nSkillPageX, pSkillData->nSkillPageY);
					UI_RECT rectFrame;
					rectFrame.m_fX1 = pos.m_fX - pGrid->m_pSkillupButtonFrame->m_fXOffset;
					rectFrame.m_fY1 = pos.m_fY - pGrid->m_pSkillupButtonFrame->m_fYOffset;
					rectFrame.m_fX2 = rectFrame.m_fX1 + pGrid->m_pSkillupButtonFrame->m_fWidth;
					rectFrame.m_fY2 = rectFrame.m_fY1 + pGrid->m_pSkillupButtonFrame->m_fHeight;
					if (UIInRect(UI_POSITION(x,y), rectFrame))
					{
						return iRow;
					}
				}
			}
		}
	}

	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnMouseHover(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);
	float x = (float)wParam;
	float y = (float)lParam;
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	// In Mythos, because our skill panes scroll, we have to be very careful about
	// clipping our mouse to them properly
	if ( pGrid->m_pParent && 
		!UIComponentCheckBounds(pGrid->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;
	y -= UIComponentGetScrollPos(pGrid).m_fY;


	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);
	float fScrollOffsetY = 0;
	if( pGrid->m_pParent )
	{
		x -= pGrid->m_Position.m_fX;
		logx -= pGrid->m_Position.m_fX;
		fScrollOffsetY = UIComponentGetScrollPos(pGrid->m_pParent).m_fY;
		logy += fScrollOffsetY;
	}
	int nSkillID = sCraftingGridGetSkillAtPosition(pGrid, logx, logy, skillpos);

	

	if (nSkillID == INVALID_LINK )
	{
		UI_COMPONENT * pBarPanel = UIComponentFindChildByName( pGrid->m_pParent, "skill bar panel" );

		nSkillID = sCraftingGridGetSkillBarAtPosition( pBarPanel, x + pos.m_fX, y + pos.m_fY, skillpos );

		if( nSkillID == INVALID_LINK )
		{
			pBarPanel = UIComponentFindChildByName( pGrid->m_pParent, "skill bar panel large" );

			nSkillID = sCraftingGridGetSkillBarAtPosition( pBarPanel, x + pos.m_fX, y + pos.m_fY, skillpos );
		}
	}
	skillpos.m_fY -= fScrollOffsetY;
	skillpos.m_fX *= UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	skillpos.m_fY *= UIGetScreenToLogRatioY(pGrid->m_bNoScale);

	
	if (nSkillID != INVALID_LINK)
	{
		const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLS, nSkillID);
		if (pSkillData)
		{
			UI_RECT rectIcon;

			rectIcon.m_fX1 = skillpos.m_fX;
			rectIcon.m_fY1 = skillpos.m_fY;
			rectIcon.m_fX2 = rectIcon.m_fX1 + pGrid->m_fCellWidth;
			rectIcon.m_fY2 = rectIcon.m_fY1 + pGrid->m_fCellHeight;
			rectIcon.m_fX1 += pos.m_fX;
			rectIcon.m_fX2 += pos.m_fX;
			rectIcon.m_fY1 += pos.m_fY;
			rectIcon.m_fY2 += pos.m_fY;
			UISetHoverTextSkill(component, UIGetControlUnit(), nSkillID, &rectIcon);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	
	UIClearHoverText();

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnMouseLeave(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	UIClearHoverText();

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);
	if (pGrid->m_nLastSkillRequestID != INVALID_LINK)
	{
		SkillStopRequest(UIGetControlUnit(), pGrid->m_nLastSkillRequestID, TRUE, TRUE);
		pGrid->m_nLastSkillRequestID = INVALID_LINK;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnLButtonDown(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);
	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( pGrid->m_pParent && !UIComponentCheckBounds(pGrid->m_pParent, x, y, NULL))
	{
		return UIMSG_RET_NOT_HANDLED;
	}


	x -= pos.m_fX;
	y -= pos.m_fY;

	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(pGrid->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pGrid->m_bNoScale);
	if( AppIsTugboat() && pGrid->m_pParent )
	{
		x -= pGrid->m_Position.m_fX;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (!pFocusUnit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UnitGetStat(pFocusUnit, STATS_CRAFTING_POINTS) > 0 )
	{
		int nSkillID( INVALID_LINK );
		UI_POSITION skillpos;
		nSkillID = sCraftingGridGetLevelUpAtPosition( pGrid, logx, logy, skillpos );
		if (nSkillID != INVALID_LINK)
		{

			MSG_CCMD_REQ_SKILL_UP msg;
			msg.wSkill = (short)nSkillID;
			c_SendMessage(CCMD_REQ_SKILL_UP, &msg);

			static int nPointSpendSound = INVALID_LINK;
			if (nPointSpendSound == INVALID_LINK)
			{
				nPointSpendSound = GlobalIndexGet(  GI_SOUND_POINTSPEND );
			}
			if (nPointSpendSound != INVALID_LINK)
			{
				c_SoundPlay( nPointSpendSound );
			}
			UIClearHoverText();


			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}	


	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);
	if (pGrid->m_nLastSkillRequestID != INVALID_LINK)
	{
		SkillStopRequest(UIGetControlUnit(), pGrid->m_nLastSkillRequestID, TRUE, TRUE);
		pGrid->m_nLastSkillRequestID = INVALID_LINK;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingGridOnRButtonUp(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_CRAFTINGGRID *pGrid = UICastToCraftingGrid(component);
	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pGrid, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (pGrid->m_nLastSkillRequestID != INVALID_LINK)
	{
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
			SkillStopRequest(pPlayer, pGrid->m_nLastSkillRequestID, TRUE, TRUE);
		pGrid->m_nLastSkillRequestID = INVALID_LINK;
	}


	return UIMSG_RET_HANDLED;
}

//////////////////////////////////////////////////////////////////////////////
// set focus unit for skill tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UISkillPointAddButtonSetVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT* pUnit = UIComponentGetFocusUnit(component);

	BOOL bVisible = FALSE;
	if (pUnit)
	{
		GAME *pGame = UnitGetGame(pUnit);
		REF( pGame );
		if ((UnitIsPlayer(pUnit) || (AppIsHellgate() && UnitGetGame(pUnit) && PetIsPlayerPet(UnitGetGame(pUnit), pUnit)))&& UnitGetStat(pUnit, STATS_SKILL_POINTS) > 0)
		{
			bVisible = TRUE;
		}
		if( AppIsTugboat() &&
			 UnitGetStat(pUnit, STATS_SKILL_POINTS_TIER_SPENT, 0 /*pGrid->m_nSkillTab*/ ) >= 30 )
		{
			bVisible = FALSE;
		}
	}


	UIComponentSetActive(component, bVisible);
	UIComponentSetVisible(component, bVisible, TRUE);
	return UIMSG_RET_HANDLED;
} // UISkillPointAddButtonSetVisible()

//////////////////////////////////////////////////////////////////////////////
// set focus unit for skill tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UISkillPointAddButtonSetFocusUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentSetFocusUnit(component, UNITID(wParam));
	UISkillPointAddButtonSetVisible(component, UIMSG_SETCONTROLSTAT, 0, 0);

	return UIMSG_RET_HANDLED;
} // UISkillPointAddButtonSetFocusUnit()

//////////////////////////////////////////////////////////////////////////////
// released the skill point tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UISkillPointAddUp(
								UI_COMPONENT* component,
								int msg,
								DWORD wParam,
								DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonUp(component, msg, wParam, lParam);

	UI_BUTTONEX *pButton = UICastToButton(component);
	ASSERT_RETVAL(pButton, UIMSG_RET_NOT_HANDLED);

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	ASSERTX_RETVAL(pFocusUnit, UIMSG_RET_END_PROCESS, "expected focus unit to add stat point to");

	if( SkillTierCanBePurchased( UIGetControlUnit(), 0/*(int)pButton->m_dwParam*/, FALSE, FALSE ) )
	{
		MSG_CCMD_REQ_TIER_UP skillmsg;
		skillmsg.wSkillTab = (short)pButton->m_dwParam;
		c_SendMessage(CCMD_REQ_TIER_UP, &skillmsg);
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
} // UISkillPointAddUp()

//////////////////////////////////////////////////////////////////////////////
// pressed the skill point tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UISkillPointAddDown(
								  UI_COMPONENT* component,
								  int msg,
								  DWORD wParam,
								  DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonDown(component, msg, wParam, lParam);

	return UIMSG_RET_HANDLED_END_PROCESS;
} // UISkillPointAddDown()	


//////////////////////////////////////////////////////////////////////////////
// display a description of tier bar bonuses
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UITierBarTooltip(
								UI_COMPONENT* component,
								int msg,
								DWORD wParam,
								DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* pPlayer = UIGetControlUnit();
	const UNIT_DATA* pUnitData = UnitGetData( pPlayer );	
	UnitSetStat( pPlayer, STATS_SKILL_LEVEL, pUnitData->nClassSkill,  UnitGetStat(pPlayer, STATS_SKILL_POINTS_TIER_SPENT, 0 ) );
	UISetHoverTextSkill(component, pPlayer, pUnitData->nClassSkill, NULL, FALSE, FALSE);

	return UIMSG_RET_NOT_HANDLED;
} // UITierBarTooltip()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillOnLButtonDown(
								   UI_COMPONENT* component,
								   int msg,
								   DWORD wParam,
								   DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nSkillID = UIGetCursorSkill();

	if (nSkillID == INVALID_ID)
	{
		// might want to put the current skill in the cursor here... later
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	const SKILL_DATA * pSkillData = SkillGetData( AppGetCltGame(), nSkillID );
	if ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_MOUSE ) ||
		!SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_REQUEST ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	// in Mythos, some skills can't be assigned to LMB
	if( AppIsTugboat() &&
		component->m_dwParam == 0 &&
		!SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_LEFT_MOUSE ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	c_SendSkillSaveAssign((WORD)nSkillID, component->m_dwParam != 0 );

	int nSoundId = GlobalIndexGet(GI_SOUND_UI_SKILL_PUTDOWN);
	if (nSoundId != INVALID_LINK)
	{
		c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
	}

	// this mirrors what's going on on the server (so we don't have to have it send a message back)
	UNIT *pPlayer = UIGetControlUnit();
	ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);
	int nCurrentConfig = UnitGetStat(pPlayer, STATS_CURRENT_WEAPONCONFIG);
	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pPlayer, nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1 );
	ASSERT_RETVAL(pTag, UIMSG_RET_NOT_HANDLED);
	if (component->m_dwParam  == 0)
		pTag->m_nSkillID[0] = nSkillID;
	else
		pTag->m_nSkillID[1] = nSkillID;

	// and clear the cursor
	UISetCursorSkill(INVALID_ID, FALSE);


	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowSkillFoldout(
								 UI_COMPONENT* component,
								 int msg,
								 DWORD wParam,
								 DWORD lParam)
{

	int nSkillID = UIGetCursorSkill();

	if (nSkillID != INVALID_ID)
	{
		// might want to put the current skill in the cursor here... later
		return UISkillOnLButtonDown( component, msg, wParam, lParam);
	}

	UI_COMPONENT* pSkillFoldoutComponent = UIComponentFindChildByName(component, "spell hotbar");
	if ( !pSkillFoldoutComponent )
	{
		pSkillFoldoutComponent = UIComponentFindChildByName(component, "spell hotbar left");
	}

	if( !UIComponentGetVisible( component ) || !UIComponentCheckBounds( component ) )
	{
		if( UIComponentGetVisible( pSkillFoldoutComponent ) )
		{
			UIComponentSetVisible( pSkillFoldoutComponent, FALSE );
		}
		return UIMSG_RET_NOT_HANDLED;
	}

	if( !UIComponentGetVisible( pSkillFoldoutComponent ) )
	{
		UIComponentSetActive( pSkillFoldoutComponent, TRUE );
		UIComponentHandleUIMessage(pSkillFoldoutComponent, UIMSG_PAINT, 0, 0);
		return UIMSG_RET_HANDLED;
	}
	else
	{
		UIComponentSetVisible( pSkillFoldoutComponent, FALSE );
	}

	return UIMSG_RET_HANDLED;
}


//////////////////////////////////////////////////////////////////////////////
// set focus unit for crafting tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UICraftingPointAddButtonSetFocusUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentSetFocusUnit(component, UNITID(wParam));
	UICraftingPointAddButtonSetVisible(component, UIMSG_SETCONTROLSTAT, 0, 0);

	return UIMSG_RET_HANDLED;
} // UICraftingPointAddButtonSetFocusUnit()

//////////////////////////////////////////////////////////////////////////////
// released the crafting point tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UICraftingPointAddUp(
								UI_COMPONENT* component,
								int msg,
								DWORD wParam,
								DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonUp(component, msg, wParam, lParam);

	UI_BUTTONEX *pButton = UICastToButton(component);
	ASSERT_RETVAL(pButton, UIMSG_RET_NOT_HANDLED);

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	ASSERTX_RETVAL(pFocusUnit, UIMSG_RET_END_PROCESS, "expected focus unit to add stat point to");

	UI_PANELEX* panel = UICastToPanel(pButton->m_pParent->m_pParent);
	int nSkillTab = sGetSkillTab( UIGetControlUnit(), (int)panel->m_nCurrentTab );
	if( CraftingTierCanBePurchased( UIGetControlUnit(), nSkillTab, FALSE, FALSE ) )
	{
		MSG_CCMD_REQ_CRAFTING_TIER_UP skillmsg;
		skillmsg.wSkillTab = (short)nSkillTab;
		c_SendMessage(CCMD_REQ_CRAFTING_TIER_UP, &skillmsg);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
} // UICraftingPointAddUp()

//////////////////////////////////////////////////////////////////////////////
// pressed the crafting point tier button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UICraftingPointAddDown(
								  UI_COMPONENT* component,
								  int msg,
								  DWORD wParam,
								  DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonDown(component, msg, wParam, lParam);

	return UIMSG_RET_HANDLED_END_PROCESS;
} // UICraftingPointAddDown()	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEatMsgHandlerCheckBoundsClearHoverText(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UISetHoverUnit( INVALID_ID, INVALID_ID );
	UIClearHoverText();
	return UIMSG_RET_HANDLED_END_PROCESS;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadInvSlot(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_INVSLOT* invslot = UICastToInvSlot(component);

	int nLastLocation = invslot->m_nInvLocation;
	XML_LOADEXCELILINK("invloc", invslot->m_nInvLocation, DATATABLE_INVLOCIDX);
	if ( invslot->m_nInvLocation == INVALID_LINK &&
		component->m_bReference )
	{
		invslot->m_nInvLocation = nLastLocation;
	}
	invslot->m_nInvUnitId = INVALID_ID;
	invslot->m_bUseUnitId = FALSE;

	XML_LOADFLOAT("cellwidth", invslot->m_fSlotScale, component->m_bReference ? invslot->m_fSlotScale : 1.0f);

	UIXMLLoadColor(xml, component, "mod_wont_fit_", invslot->m_dwModWontFitColor, 0x9e200000);
	UIXMLLoadColor(xml, component, "mod_wont_go_in_item_", invslot->m_dwModWontGoInItemColor, 0x9e202020);

	UIX_TEXTURE* texture = UIComponentGetTexture(invslot);
	if (texture)
	{
		UI_TEXTURE_FRAME* pLitFrame = invslot->m_pLitFrame;
		UI_TEXTURE_FRAME* pUseableBorderFrame = invslot->m_pUseableBorderFrame;
		UI_TEXTURE_FRAME* pSocketFrame = invslot->m_pSocketFrame;
		UI_TEXTURE_FRAME* pModFrame = invslot->m_pModFrame;

		XML_LOADFRAME("litframe",		texture, invslot->m_pLitFrame, "");
		XML_LOADFRAME("useableborder",	texture, invslot->m_pUseableBorderFrame, "");
		XML_LOADFRAME("socketframe",	texture, invslot->m_pSocketFrame, "");
		XML_LOADFRAME("modframe",		texture, invslot->m_pModFrame, "");

		if( component->m_bReference )
		{
			if( !invslot->m_pLitFrame )				invslot->m_pLitFrame = pLitFrame;
			if( !invslot->m_pUseableBorderFrame )	invslot->m_pUseableBorderFrame = pUseableBorderFrame;
			if( !invslot->m_pSocketFrame )			invslot->m_pSocketFrame = pSocketFrame;
			if( !invslot->m_pModFrame )				invslot->m_pModFrame = pModFrame;
		}
	}

	invslot->m_ItemAdj.fScale = 1.0f;
	invslot->m_tItemAdjAnimInfo.m_dwAnimTicket = INVALID_ID;
	XML_LOADFLOAT("model_border_x", invslot->m_fModelBorderX, 0.0f);
	XML_LOADFLOAT("model_border_y", invslot->m_fModelBorderY, 0.0f);

	UI_COMPONENT *pIconPanel = UIComponentFindChildByName(component, "icon panel");
	UIX_TEXTURE *pIconTexture = (pIconPanel ? UIComponentGetTexture(pIconPanel) : NULL);
	if (pIconTexture)
	{
		UI_TEXTURE_FRAME* pUnidentifiedIconFrame =	invslot->m_pUnidentifiedIconFrame;
		UI_TEXTURE_FRAME* pBadClassIconFrame =		invslot->m_pBadClassIconFrame;
		UI_TEXTURE_FRAME* pBadClassUnIDIconFrame =	invslot->m_pBadClassUnIDIconFrame;
		UI_TEXTURE_FRAME* pLowStatsIconFrame =		invslot->m_pLowStatsIconFrame;

		XML_LOADFRAME("lowstatsicon",		pIconTexture, invslot->m_pLowStatsIconFrame, "");
		XML_LOADFRAME("unidentifiedicon",	pIconTexture, invslot->m_pUnidentifiedIconFrame, "");
		XML_LOADFRAME("badclassicon",		pIconTexture, invslot->m_pBadClassIconFrame, "");
		XML_LOADFRAME("badclassuicon",		pIconTexture, invslot->m_pBadClassUnIDIconFrame, "");
		XML_LOADFRAME("lowstatsicon",		pIconTexture, invslot->m_pLowStatsIconFrame, "");

		if( component->m_bReference )
		{
			if( !invslot->m_pLowStatsIconFrame )	invslot->m_pLowStatsIconFrame = pLowStatsIconFrame;
			if( !invslot->m_pUnidentifiedIconFrame )invslot->m_pUseableBorderFrame = pUnidentifiedIconFrame;
			if( !invslot->m_pBadClassIconFrame )	invslot->m_pUseableBorderFrame = pBadClassIconFrame;
			if( !invslot->m_pBadClassUnIDIconFrame )invslot->m_pBadClassUnIDIconFrame = pBadClassUnIDIconFrame;
			if( !invslot->m_pLowStatsIconFrame )	invslot->m_pLowStatsIconFrame = pLowStatsIconFrame;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadSkillsGrid(
						 CMarkup& xml,
						 UI_COMPONENT* component, 
						 const UI_XML & tUIXml)
{
	UI_SKILLSGRID* pSkillsGrid = UICastToSkillsGrid(component);

	XML_LOADEXCELILINK("invloc", pSkillsGrid->m_nInvLocation, DATATABLE_INVLOCIDX);
	XML_LOAD("skilltab", pSkillsGrid->m_dwData, DWORD, INVALID_LINK);
	XML_LOAD("ishotbar", pSkillsGrid->m_bIsHotBar, BOOL, FALSE );
	pSkillsGrid->m_nSkillTab = INVALID_LINK;
	pSkillsGrid->m_bLookupTabNum = TRUE;
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayerUnit = GameGetControlUnit(pGame);
		if (pPlayerUnit)
		{
			pSkillsGrid->m_nSkillTab = sGetSkillTab( pPlayerUnit, (int)pSkillsGrid->m_dwData );
			if ( pSkillsGrid->m_nSkillTab != INVALID_ID )
				pSkillsGrid->m_bLookupTabNum = FALSE;
		}
	}

	XML_LOADINT("cellsx", pSkillsGrid->m_nCellsX, -1);
	XML_LOADINT("cellsy", pSkillsGrid->m_nCellsY, -1);
	XML_LOADFLOAT("cellwidth", pSkillsGrid->m_fCellWidth, 0.0f);
	XML_LOADFLOAT("cellheight", pSkillsGrid->m_fCellHeight, 0.0f);
	XML_LOADFLOAT("cellborder", pSkillsGrid->m_fCellBorderX, 0.0f);
	XML_LOADFLOAT("cellborderx", pSkillsGrid->m_fCellBorderX, pSkillsGrid->m_fCellBorderX);
	pSkillsGrid->m_fCellBorderY = pSkillsGrid->m_fCellBorderX;
	XML_LOADFLOAT("cellbordery", pSkillsGrid->m_fCellBorderY, pSkillsGrid->m_fCellBorderY);
	pSkillsGrid->m_fCellWidth *= tUIXml.m_fXmlWidthRatio;
	pSkillsGrid->m_fCellHeight *= tUIXml.m_fXmlHeightRatio;
	pSkillsGrid->m_fCellBorderX *= tUIXml.m_fXmlWidthRatio;
	XML_LOADFLOAT("iconwidth", pSkillsGrid->m_fIconWidth, -1.0f);
	XML_LOADFLOAT("iconheight", pSkillsGrid->m_fIconHeight, -1.0f);
	pSkillsGrid->m_fIconWidth *= tUIXml.m_fXmlWidthRatio;
	pSkillsGrid->m_fIconHeight *= tUIXml.m_fXmlHeightRatio;

	XML_LOADFLOAT("leveltext_offset", pSkillsGrid->m_fLevelTextOffset, 0.0f);

	// I added support for different borderwidths vertically and horizontally,
	// but multiplying the yborder by heightratio might mess up hellgate -
	// so leaving it for now
	if( AppIsTugboat() )
	{
		pSkillsGrid->m_fCellBorderY *= tUIXml.m_fXmlHeightRatio;
	}
	else if( AppIsHellgate() )
	{
		pSkillsGrid->m_fCellBorderY *= tUIXml.m_fXmlWidthRatio;
	}

	pSkillsGrid->m_pBorderFrame = NULL;
	pSkillsGrid->m_pBkgndFrame = NULL;
	pSkillsGrid->m_pBkgndFramePassive = NULL;
	UIX_TEXTURE* texture = UIComponentGetTexture(pSkillsGrid);
	if (texture)
	{
		XML_LOADFRAME("borderframe", texture, pSkillsGrid->m_pBorderFrame, "");
		XML_LOADFRAME("bkgndframe", texture, pSkillsGrid->m_pBkgndFrame, "");
		XML_LOADFRAME("bkgndframepassive", texture, pSkillsGrid->m_pBkgndFramePassive, "");
		XML_LOADFRAME("selectedframe", texture, pSkillsGrid->m_pSelectedFrame, "");
		XML_LOADFRAME("skillupborder", texture, pSkillsGrid->m_pSkillupBorderFrame, "");
		XML_LOADFRAME("skillupbutton", texture, pSkillsGrid->m_pSkillupButtonFrame, "");
		XML_LOADFRAME("skillupbuttondown", texture, pSkillsGrid->m_pSkillupButtonDownFrame, "");
		XML_LOADFRAME("skillshiftbutton", texture, pSkillsGrid->m_pSkillShiftButtonFrame, "");
		XML_LOADFRAME("skillshiftbuttondown", texture, pSkillsGrid->m_pSkillShiftButtonDownFrame, "");
		XML_LOADFRAME("skillpoint", texture, pSkillsGrid->m_pSkillPoint, "skill gem");
	}

	pSkillsGrid->m_pIconPanel = (UI_PANELEX *) UIComponentCreateNew(pSkillsGrid, UITYPE_PANEL, TRUE);
	ASSERT_RETFALSE(pSkillsGrid->m_pIconPanel);
	pSkillsGrid->m_pIconPanel->m_fWidth = pSkillsGrid->m_fWidth;
	pSkillsGrid->m_pIconPanel->m_fHeight = pSkillsGrid->m_fHeight;
	pSkillsGrid->m_pIconPanel->m_nRenderSection = pSkillsGrid->m_nRenderSection+1;

	if( AppIsTugboat() )
	{
		pSkillsGrid->m_pHighlightPanel = (UI_PANELEX *) UIComponentCreateNew(pSkillsGrid, UITYPE_PANEL, TRUE);
		ASSERT_RETFALSE(pSkillsGrid->m_pHighlightPanel);
		pSkillsGrid->m_pHighlightPanel->m_fWidth = pSkillsGrid->m_fWidth;
		pSkillsGrid->m_pHighlightPanel->m_fHeight = pSkillsGrid->m_fHeight;
		pSkillsGrid->m_pHighlightPanel->m_nRenderSection = pSkillsGrid->m_nRenderSection+1;
		UIComponentThrobStart( pSkillsGrid->m_pHighlightPanel, 0xffffffff, 2000 );

	}
	else
	{
		pSkillsGrid->m_pHighlightPanel = NULL;
	}

	pSkillsGrid->m_nSkillupSkill = INVALID_ID;

	XML_LOADEXCELILINK("shift_toggle_on_sound", pSkillsGrid->m_nShiftToggleOnSound, DATATABLE_SOUNDS);
	if (pSkillsGrid->m_nShiftToggleOnSound != INVALID_ID)
		c_SoundLoad(pSkillsGrid->m_nShiftToggleOnSound);	// be sure to pre-load the sound so it plays the first time
	XML_LOADEXCELILINK("shift_toggle_off_sound", pSkillsGrid->m_nShiftToggleOffSound, DATATABLE_SOUNDS);
	if (pSkillsGrid->m_nShiftToggleOffSound != INVALID_ID)
		c_SoundLoad(pSkillsGrid->m_nShiftToggleOffSound);	// be sure to pre-load the sound so it plays the first time

	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadCraftingGrid(
						 CMarkup& xml,
						 UI_COMPONENT* component, 
						 const UI_XML & tUIXml)
{
	UI_CRAFTINGGRID* pCraftingGrid = UICastToCraftingGrid(component);

	XML_LOADEXCELILINK("invloc", pCraftingGrid->m_nInvLocation, DATATABLE_INVLOCIDX);
	XML_LOAD("skilltab", pCraftingGrid->m_dwData, DWORD, INVALID_LINK);
	pCraftingGrid->m_nSkillTab = INVALID_LINK;
	pCraftingGrid->m_bLookupTabNum = TRUE;
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayerUnit = GameGetControlUnit(pGame);
		if (pPlayerUnit)
		{
			pCraftingGrid->m_nSkillTab = sGetSkillTab( pPlayerUnit, (int)pCraftingGrid->m_dwData );
			if ( pCraftingGrid->m_nSkillTab != INVALID_ID )
				pCraftingGrid->m_bLookupTabNum = FALSE;
		}
	}

	XML_LOADINT("cellsx", pCraftingGrid->m_nCellsX, -1);
	XML_LOADINT("cellsy", pCraftingGrid->m_nCellsY, -1);
	XML_LOADFLOAT("cellwidth", pCraftingGrid->m_fCellWidth, 0.0f);
	XML_LOADFLOAT("cellheight", pCraftingGrid->m_fCellHeight, 0.0f);
	XML_LOADFLOAT("cellborder", pCraftingGrid->m_fCellBorderX, 0.0f);
	XML_LOADFLOAT("cellborderx", pCraftingGrid->m_fCellBorderX, pCraftingGrid->m_fCellBorderX);
	pCraftingGrid->m_fCellBorderY = pCraftingGrid->m_fCellBorderX;
	XML_LOADFLOAT("cellbordery", pCraftingGrid->m_fCellBorderY, pCraftingGrid->m_fCellBorderY);
	pCraftingGrid->m_fCellWidth *= tUIXml.m_fXmlWidthRatio;
	pCraftingGrid->m_fCellHeight *= tUIXml.m_fXmlHeightRatio;
	pCraftingGrid->m_fCellBorderX *= tUIXml.m_fXmlWidthRatio;
	XML_LOADFLOAT("iconwidth", pCraftingGrid->m_fIconWidth, -1.0f);
	XML_LOADFLOAT("iconheight", pCraftingGrid->m_fIconHeight, -1.0f);
	pCraftingGrid->m_fIconWidth *= tUIXml.m_fXmlWidthRatio;
	pCraftingGrid->m_fIconHeight *= tUIXml.m_fXmlHeightRatio;

	XML_LOADFLOAT("leveltext_offset", pCraftingGrid->m_fLevelTextOffset, 0.0f);

	// I added support for different borderwidths vertically and horizontally,
	// but multiplying the yborder by heightratio might mess up hellgate -
	// so leaving it for now
	if( AppIsTugboat() )
	{
		pCraftingGrid->m_fCellBorderY *= tUIXml.m_fXmlHeightRatio;
	}
	else if( AppIsHellgate() )
	{
		pCraftingGrid->m_fCellBorderY *= tUIXml.m_fXmlWidthRatio;
	}

	pCraftingGrid->m_pBorderFrame = NULL;
	pCraftingGrid->m_pBkgndFrame = NULL;
	pCraftingGrid->m_pBkgndFramePassive = NULL;
	UIX_TEXTURE* texture = UIComponentGetTexture(pCraftingGrid);
	if (texture)
	{
		XML_LOADFRAME("borderframe", texture, pCraftingGrid->m_pBorderFrame, "");
		XML_LOADFRAME("bkgndframe", texture, pCraftingGrid->m_pBkgndFrame, "");
		XML_LOADFRAME("bkgndframepassive", texture, pCraftingGrid->m_pBkgndFramePassive, "");
		XML_LOADFRAME("selectedframe", texture, pCraftingGrid->m_pSelectedFrame, "");
		XML_LOADFRAME("skillupborder", texture, pCraftingGrid->m_pSkillupBorderFrame, "");
		XML_LOADFRAME("skillupbutton", texture, pCraftingGrid->m_pSkillupButtonFrame, "");
		XML_LOADFRAME("skillupbuttondown", texture, pCraftingGrid->m_pSkillupButtonDownFrame, "");
		XML_LOADFRAME("skillshiftbutton", texture, pCraftingGrid->m_pSkillShiftButtonFrame, "");
		XML_LOADFRAME("skillshiftbuttondown", texture, pCraftingGrid->m_pSkillShiftButtonDownFrame, "");
		XML_LOADFRAME("skillpoint", texture, pCraftingGrid->m_pSkillPoint, "skill gem");
	}

	pCraftingGrid->m_pIconPanel = (UI_PANELEX *) UIComponentCreateNew(pCraftingGrid, UITYPE_PANEL, TRUE);
	ASSERT_RETFALSE(pCraftingGrid->m_pIconPanel);
	pCraftingGrid->m_pIconPanel->m_fWidth = pCraftingGrid->m_fWidth;
	pCraftingGrid->m_pIconPanel->m_fHeight = pCraftingGrid->m_fHeight;
	pCraftingGrid->m_pIconPanel->m_nRenderSection = pCraftingGrid->m_nRenderSection+1;

	if( AppIsTugboat() )
	{
		pCraftingGrid->m_pHighlightPanel = (UI_PANELEX *) UIComponentCreateNew(pCraftingGrid, UITYPE_PANEL, TRUE);
		ASSERT_RETFALSE(pCraftingGrid->m_pHighlightPanel);
		pCraftingGrid->m_pHighlightPanel->m_fWidth = pCraftingGrid->m_fWidth;
		pCraftingGrid->m_pHighlightPanel->m_fHeight = pCraftingGrid->m_fHeight;
		pCraftingGrid->m_pHighlightPanel->m_nRenderSection = pCraftingGrid->m_nRenderSection+1;
		UIComponentThrobStart( pCraftingGrid->m_pHighlightPanel, 0xffffffff, 2000 );

	}
	else
	{
		pCraftingGrid->m_pHighlightPanel = NULL;
	}

	pCraftingGrid->m_nSkillupSkill = INVALID_ID;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadTradesmanSheet(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_TRADESMANSHEET* pTradesmanSheet = UICastToTradesmanSheet(component);
	XML_LOADEXCELILINK("invloc", pTradesmanSheet->m_nInvLocation, DATATABLE_INVLOCIDX);


	XML_LOADINT("cellsy", pTradesmanSheet->m_nCellsY, -1);
	XML_LOADFLOAT("cellheight", pTradesmanSheet->m_fCellHeight, 0.0f);
	pTradesmanSheet->m_fCellHeight *= tUIXml.m_fXmlHeightRatio;
	pTradesmanSheet->m_pBkgndFrame = NULL;
	UIX_TEXTURE* texture = UIComponentGetTexture(pTradesmanSheet);
	if (texture)
	{
		XML_LOADFRAME("bkgndframe", texture, pTradesmanSheet->m_pBkgndFrame, "");
		XML_LOADFRAME("createbutton", texture, pTradesmanSheet->m_pCreateButtonFrame, "tradesman create button");

	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadUnitNameDisplay(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	ASSERT_RETFALSE(component);
	UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay(component);
	ASSERT_RETFALSE(pNameDisp);

	pNameDisp->m_idUnitNameUnderCursor = INVALID_ID;

	XML_LOADFLOAT("maxitemnamewidth", pNameDisp->m_fMaxItemNameWidth, 0.0f);

	UIX_TEXTURE* texture = UIComponentGetTexture(pNameDisp);

	if (texture)
	{
		XML_LOADFRAME("itemborderframe",	texture, pNameDisp->m_pItemBorderFrame, "");
		XML_LOADFRAME("unidentifiedicon",	texture, pNameDisp->m_pUnidentifiedIconFrame, "");
		XML_LOADFRAME("badclassicon",		texture, pNameDisp->m_pBadClassIconFrame, "");
		XML_LOADFRAME("badclassuicon",		texture, pNameDisp->m_pBadClassUnIDIconFrame, "");
		XML_LOADFRAME("lowstatsicon",		texture, pNameDisp->m_pLowStatsIconFrame, "");
		XML_LOADFRAME("hardcoreicon",		texture, pNameDisp->m_pHardcoreFrame, "");
		XML_LOADFRAME("roleplayicon",		texture, pNameDisp->m_pLeagueFrame, "");
		XML_LOADFRAME("eliteicon",			texture, pNameDisp->m_pEliteFrame, "");
		XML_LOADFRAME("pvponlyicon",		texture, pNameDisp->m_pPVPOnlyFrame, "");
	}

	if (pNameDisp->m_pPickupLine == NULL)
		pNameDisp->m_pPickupLine = MALLOC_NEW(NULL, UI_LINE);		// KCK: Grr. Stupid memory macros. I should just override the new operator.
	
	pNameDisp->m_pPickupLine->SetText(GlobalStringGet( GS_UI_PICK_UP_TEXT ));
	UIReplaceStringTags(pNameDisp, pNameDisp->m_pPickupLine);

	if (!component->m_bReference)
	{
		pNameDisp->m_nDisplayArea = INVALID_LINK;
	}
	extern STR_DICT tooltipareas[];
	XML_LOADENUM("chardisplayarea", pNameDisp->m_nDisplayArea, tooltipareas, -1);
	if (pNameDisp->m_nDisplayArea != -1)
	{
		pNameDisp->m_eTable = DATATABLE_CHARDISPLAY;
	}
	UIXMLLoadColor(xml, component, "hardcoreicon", pNameDisp->m_dwHardcoreFrameColor, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, component, "roleplayicon", pNameDisp->m_dwLeagueFrameColor, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, component, "eliteicon", pNameDisp->m_dwEliteFrameColor, GFXCOLOR_WHITE);

	XML_LOADFLOAT("player_info_spacing", pNameDisp->m_fPlayerInfoSpacing, 0.0f);
	XML_LOADFLOAT("player_badge_spacing", pNameDisp->m_fPlayerBadgeSpacing, 0.0f);
	XML_LOADINT("player_info_fontsize", pNameDisp->m_nPlayerInfoFontsize, UIComponentGetFontSize(pNameDisp));
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UINameDisplayFree(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);

	UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay(pComponent);
	ASSERT_RETURN(pNameDisp);

	if (pNameDisp->m_pPickupLine)
	{
		FREE_DELETE(NULL, pNameDisp->m_pPickupLine, UI_LINE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadActiveSlot(
	CMarkup &xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_ACTIVESLOT* pSlot = UICastToActiveSlot(component);

	pSlot->m_nSlotNumber = -1;
	char szCommandName[256];
	XML_LOADSTRING("command", szCommandName, "", arrsize(szCommandName));
	const KEY_COMMAND *pKey = InputGetKeyCommand(szCommandName);
	if (pKey)
	{
		pSlot->m_nKeyCommand = pKey->nCommand;
		pSlot->m_nSlotNumber = pKey->dwParam;
	}

	if (!pSlot->m_bReference)
		pSlot->m_pLitFrame = NULL;

	UIX_TEXTURE* texture = UIComponentGetTexture(pSlot);
	if (texture)
	{
		XML_LOADFRAME("litframe", texture, pSlot->m_pLitFrame, "");
	}

	pSlot->m_pIconPanel = NULL;
	UI_COMPONENT *pChild = pSlot->m_pFirstChild;
	while(pChild)
	{
		if (PStrCmp(pChild->m_szName, "icon panel")==0)
		{
			pSlot->m_pIconPanel = pChild;
		}
		pChild = pChild->m_pNextSibling;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesComplex(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	
	// init globals
	memset(pgdwGameTickStateEnds, 0, sizeof(DWORD) * MAX_STATES_DISPLAY_ITEMS);
	memset(pgnTrackingStates, -1, sizeof(int) * MAX_STATES_DISPLAY_ITEMS);
	
	//					   struct				type						name				load function				free function
	UI_REGISTER_COMPONENT( UI_INVGRID,			UITYPE_INVGRID,				"invgrid",			UIXmlLoadInvGrid,			UIInvGridFree	);
	UI_REGISTER_COMPONENT( UI_INVSLOT,			UITYPE_INVSLOT,				"invslot",			UIXmlLoadInvSlot,			NULL			);
	UI_REGISTER_COMPONENT( UI_SKILLSGRID,		UITYPE_SKILLSGRID,			"skillsgrid",		UIXmlLoadSkillsGrid,		NULL			);
	UI_REGISTER_COMPONENT( UI_UNIT_NAME_DISPLAY,UITYPE_UNIT_NAME_DISPLAY,	"unitdisplay",		UIXmlLoadUnitNameDisplay,	UINameDisplayFree	);
	UI_REGISTER_COMPONENT( UI_ACTIVESLOT,		UITYPE_ACTIVESLOT,			"activebarslot",	UIXmlLoadActiveSlot,		NULL			);
	UI_REGISTER_COMPONENT( UI_TRADESMANSHEET,	UITYPE_TRADESMANSHEET,		"tradesmansheet",	UIXmlLoadTradesmanSheet,	UITradesmanSheetFree			);
	UI_REGISTER_COMPONENT( UI_CRAFTINGGRID,		UITYPE_CRAFTINGGRID,		"craftinggrid",		UIXmlLoadCraftingGrid,		NULL			);

	UIMAP(UITYPE_INVGRID,			UIMSG_PAINT,				UIInvGridOnPaint);
	UIMAP(UITYPE_INVGRID,			UIMSG_INVENTORYCHANGE,		UIInvGridOnInventoryChange);
	UIMAP(UITYPE_INVGRID,			UIMSG_MOUSEOVER,			UIInvGridOnMouseMove);
	UIMAP(UITYPE_INVGRID,			UIMSG_MOUSEHOVER,			UIInvGridOnMouseHover);
	UIMAP(UITYPE_INVGRID,			UIMSG_MOUSEHOVERLONG,		UIInvGridOnMouseHover);
	UIMAP(UITYPE_INVGRID,			UIMSG_MOUSELEAVE,			UIInvGridOnMouseLeave);
	UIMAP(UITYPE_INVGRID,			UIMSG_LBUTTONDOWN,			UIInvGridOnLButtonDown);
	UIMAP(UITYPE_INVGRID,   		UIMSG_RBUTTONDOWN,			UIInvGridOnRButtonDown);
	UIMAP(UITYPE_INVGRID,   		UIMSG_RBUTTONUP,			UIInvGridOnRButtonUp);
	UIMAP(UITYPE_INVGRID,   		UIMSG_POSTINACTIVATE,		UIInvGridOnPostInactivate);

	UIMAP(UITYPE_INVSLOT,			UIMSG_PAINT,				UIInvSlotOnPaint);
	UIMAP(UITYPE_INVSLOT,			UIMSG_INVENTORYCHANGE,		UIInvSlotOnInventoryChange);
	UIMAP(UITYPE_INVSLOT,			UIMSG_MOUSEOVER,			UIInvSlotOnMouseMove);
	UIMAP(UITYPE_INVSLOT,			UIMSG_MOUSEHOVER,			UIInvSlotOnMouseHover);
	UIMAP(UITYPE_INVSLOT,			UIMSG_MOUSEHOVERLONG,		UIInvSlotOnMouseHover);
	UIMAP(UITYPE_INVSLOT,			UIMSG_MOUSELEAVE,			UIInvSlotOnMouseLeave);
	UIMAP(UITYPE_INVSLOT,			UIMSG_LBUTTONDOWN,			UIInvSlotOnLButtonDown);
	UIMAP(UITYPE_INVSLOT,			UIMSG_RBUTTONDOWN,			UIInvSlotOnRButtonDown);
	UIMAP(UITYPE_INVSLOT,			UIMSG_SETFOCUSUNIT,			UIInvSlotOnSetFocusUnit);

	UIMAP(UITYPE_SKILLSGRID,		UIMSG_PAINT,		    	UISkillsGridOnPaint);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_MOUSEOVER,		    UISkillsGridOnPaint);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_MOUSEHOVER,    		UISkillsGridOnMouseHover);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_LBUTTONDOWN,	    	UISkillsGridOnLButtonDown);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_RBUTTONDOWN,	    	UISkillsGridOnRButtonDown);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_RBUTTONUP,	    	UISkillsGridOnRButtonUp);
//	UIMAP(UITYPE_SKILLSGRID,		UIMSG_SETCONTROLSTAT,    	UISkillsGridOnPaint);						// <-- We can't do this.  To many stats change too much.  We're going to have to set it to update on certain stats on a case by case basis in the XML.
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_MOUSELEAVE,    		UISkillsGridOnMouseLeave);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_SETCONTROLUNIT,		UISkillsGridOnSetControlUnit);
	UIMAP(UITYPE_SKILLSGRID,		UIMSG_POSTINACTIVATE,		UISkillsGridOnPostInactivate);

	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_PAINT,		    	UICraftingGridOnPaint);
	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_MOUSEOVER,		    UICraftingGridOnPaint);
	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_MOUSEHOVER,    		UICraftingGridOnMouseHover);
	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_LBUTTONDOWN,	    	UICraftingGridOnLButtonDown);
	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_MOUSELEAVE,    		UICraftingGridOnMouseLeave);
	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_SETCONTROLUNIT,		UICraftingGridOnSetControlUnit);
	UIMAP(UITYPE_CRAFTINGGRID,		UIMSG_POSTINACTIVATE,		UICraftingGridOnPostInactivate);

	UIMAP(UITYPE_TRADESMANSHEET,		UIMSG_PAINT,		    	UITradesmanSheetOnPaint);
	UIMAP(UITYPE_TRADESMANSHEET,		UIMSG_MOUSEOVER,		    UITradesmanSheetOnPaint);
	UIMAP(UITYPE_TRADESMANSHEET,		UIMSG_MOUSEHOVER,    		UITradesmanSheetOnMouseHover);
	UIMAP(UITYPE_TRADESMANSHEET,		UIMSG_LBUTTONDOWN,	    	UITradesmanSheetOnLButtonDown);
	UIMAP(UITYPE_TRADESMANSHEET,		UIMSG_MOUSELEAVE,    		UITradesmanSheetOnMouseLeave);
	UIMAP(UITYPE_TRADESMANSHEET,		UIMSG_SETCONTROLUNIT,		UITradesmanSheetOnSetControlUnit);

	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_LBUTTONDOWN,			UIActiveSlotOnLButtonDown);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_RBUTTONDOWN,			UIActiveSlotOnRButtonDown);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_RBUTTONCLICK,			UIActiveSlotOnRButtonUp);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_PAINT,				UIActiveSlotOnPaint);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_INVENTORYCHANGE,		UIActiveSlotOnPaint);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_MOUSEOVER,			UIActiveSlotOnMouseOver);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_MOUSELEAVE,			UIActiveSlotOnMouseLeave);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_MOUSEHOVER,			UIActiveSlotOnMouseHover);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_MOUSEHOVERLONG,		UIActiveSlotOnMouseHover);
//	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_SETCONTROLSTATE,		UIActiveSlotShowBoost);
	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_SETCONTROLUNIT,		UIActiveSlotOnPaint);
//	UIMAP(UITYPE_ACTIVESLOT,		UIMSG_SETCONTROLSTAT,		UIActiveSlotOnPaint);

	UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name						function pointer
		{ "UIInvGridOnPaint",					UIInvGridOnPaint },
		{ "UIActiveSlotOnPaint",				UIActiveSlotOnPaint },
		{ "UIActiveSlotLabelOnPaint",			UIActiveSlotLabelOnPaint },
		{ "UISkillIconLabelOnPaint",			UISkillIconLabelOnPaint },
		{ "UIItemIconLabelOnPaint",				UIItemIconLabelOnPaint },
		{ "UISkillPageOnPaint",					UISkillPageOnPaint },
		{ "UISkillsGridOnPaint",				UISkillsGridOnPaint },
		{ "UISkillsGridOnMouseHover",			UISkillsGridOnMouseHover },
		{ "UISkillsGridOnLButtonDown",			UISkillsGridOnLButtonDown },
		{ "UISkillsGridOnRButtonDown",			UISkillsGridOnRButtonDown },
		{ "UIStatesDisplayOnPaint",				UIStatesDisplayOnPaint },
		{ "UIStatesDisplayOnMouseHover",		UIStatesDisplayOnMouseHover },
		{ "UIStatesDisplayOnMouseLeave",		UIStatesDisplayOnMouseLeave },
		{ "UIPartyListOnPaint",					UIPartyListOnPaint },
		{ "UIPartyListPanelOnPostActivate",		UIPartyListPanelOnPostActivate },
		{ "UIPartyCreateBtnOnClick",			UIPartyCreateBtnOnClick },
		{ "UIPartyListHeaderPartyNameOnClick",	UIPartyListHeaderPartyNameOnClick },
		{ "UIPartyListHeaderPartyNumberOnClick",UIPartyListHeaderPartyNumberOnClick },
		{ "UIPartyListHeaderPartyRangeOnClick",	UIPartyListHeaderPartyRangeOnClick },

		{ "UIPlayerMatchSetPartyMode",			UIPlayerMatchSetPartyMode },
		{ "UIPlayerMatchSetPlayerMode",			UIPlayerMatchSetPlayerMode },
		{ "UIPlayerMatchSetMatchMode",			UIPlayerMatchSetMatchMode },
		{ "UIPlayerMatchOnPostActivate",		UIPlayerMatchOnPostActivate },
		{ "UIPlayerMatchOnPaint",				UIPlayerMatchOnPaint },
		{ "UIPlayerMatchOnLButtonDown",			UIPlayerMatchOnLButtonDown },
		{ "UIPlayerMatchOnMouseOver",			UIPlayerMatchOnMouseOver },
		{ "UIScoreboardOnPaint",				UIScoreboardOnPaint },
		{ "UIScoreboardOnPostInvisible",		UIScoreboardOnPostInvisible },
		{ "UIScoreboardOnPostVisible",			UIScoreboardOnPostVisible },

		{ "UIGuildOnlyButtonClicked",			UIGuildOnlyButtonClicked },
		{ "UIPartyAction1BtnOnClick",			UIPartyAction1BtnOnClick },
		{ "UIPartyAction2BtnOnClick",			UIPartyAction2BtnOnClick },
		{ "UIPartyAction3BtnOnClick",			UIPartyAction3BtnOnClick },

		{ "UICloseParent",						UICloseParent },
		{ "UIPartyListOnLButtonDown",			UIPartyListOnLButtonDown },
		{ "UIPartyListOnMouseOver",				UIPartyListOnMouseOver },
		{ "UIPartyMembersOnPaint",				UIPartyMembersOnPaint },
		{ "UIWaypointsTransportOnClick",		UIWaypointsTransportOnClick },
		{ "UIPartyInviteBtnOnClick",			UIPartyInviteBtnOnClick },
		{ "UIPartyJoinBtnOnClick",				UIPartyJoinBtnOnClick },
		{ "UIPartyWhisperBtnOnClick",			UIPartyWhisperBtnOnClick },
		{ "UIOnPostActivateSkillMap",			UIOnPostActivateSkillMap },
		{ "UIOnPostInactivateSkillMap",			UIOnPostInactivateSkillMap },
		{ "UIMerchantInvGridOnSetControlUnit",	UIMerchantInvGridOnSetControlUnit },
		{ "UIWaypointsWaypointOnClick",			UIWaypointsWaypointOnClick },
		{ "UIWaypointScrollUpOnClick",			UIWaypointScrollUpOnClick },
		{ "UIWaypointScrollDownOnClick",		UIWaypointScrollDownOnClick },
		{ "UIWaypointsOnMouseWheel",			UIWaypointsOnMouseWheel },
		{ "UICraftingPageOnPaint",				UICraftingPageOnPaint },
		{ "UICraftingGridOnPaint",				UICraftingGridOnPaint },
		{ "UICraftingGridOnMouseHover",			UICraftingGridOnMouseHover },
		{ "UICraftingGridOnLButtonDown",		UICraftingGridOnLButtonDown },
		{ "UIShowSkillFoldout",					UIShowSkillFoldout },
		{ "UISkillPointAddButtonSetFocusUnit",	UISkillPointAddButtonSetFocusUnit },
		{ "UISkillPointAddButtonSetVisible",	UISkillPointAddButtonSetVisible },
		{ "UISkillPointAddDown",				UISkillPointAddDown },
		{ "UISkillPointAddUp",					UISkillPointAddUp },
		{ "UICraftingPointAddButtonSetFocusUnit",UICraftingPointAddButtonSetFocusUnit },
		{ "UICraftingPointAddButtonSetVisible",	UICraftingPointAddButtonSetVisible },
		{ "UICraftingPointAddDown",				UICraftingPointAddDown },
		{ "UICraftingPointAddUp",				UICraftingPointAddUp },
		{ "UISkillOnLButtonDown",				UISkillOnLButtonDown },
		{ "UITierBarTooltip",					UITierBarTooltip },
		{ "UIInstanceComboOnSelect",			UIInstanceComboOnSelect },
		{ "UIEatMsgHandlerCheckBoundsClearHoverText",			UIEatMsgHandlerCheckBoundsClearHoverText },
		{ "UIAnchorstoneButtonDown",			UIAnchorstoneButtonDown },
		{ "UIAnchorstoneCancelButtonDown",		UIAnchorstoneCancelButtonDown },
		{ "UIAnchorstoneLinkButtonDown",		UIAnchorstoneLinkButtonDown },
		{ "UIRunegateButtonDown",				UIRunegateButtonDown },
		{ "UIRunegateCancelButtonDown",			UIRunegateCancelButtonDown },
		

	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif //!SERVER_VERSION

