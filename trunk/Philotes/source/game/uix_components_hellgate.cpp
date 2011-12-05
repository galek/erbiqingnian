//----------------------------------------------------------------------------
// uix_components_hellgate.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix.h"
#include "uix_graphic.h"
#include "uix_priv.h"
#include "uix_components_hellgate.h"
#include "uix_graphic.h"
#include "uix_scheduler.h"
#include "dictionary.h"
#include "markup.h"
#include "stringtable.h"
#include "excel.h"
#include "prime.h"
#include "game.h"
#include "units.h"
#include "unittag.h"
#include "player.h"
#include "e_main.h"
#include "e_model.h"
#include "interact.h"
#include "inventory.h"
#include "items.h"
#include "c_message.h"
#include "quests.h"
#include "c_input.h"
#include "skills.h"
#include "skills_shift.h"
#include "fontcolor.h"
#include "c_font.h"
#include "camera_priv.h"
#include "c_camera.h"
#include "npc.h"
#include "c_quests.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "Quest.h"
#include "states.h"
#include "gameunits.h"
#include "wardrobe.h"
#include "complexmaths.h"
#include "e_settings.h"
#include "uix_quests.h"
#include "weaponconfig.h"
#include "missiles.h"
#include "quest_tutorial.h"
#include "serverlist.h"
#include "region.h"

// vvv Tugboat
#include "chatserver.h"
#include "s_partyCache.h"
#include "svrstd.h"
#include "levelareas.h"
#include "c_GameMapsClient.h"
#include "c_LevelAreasClient.h"
#include "LevelZones.h"
#include "LevelAreasKnownIterator.h"
#include "LevelAreaLinker.h"
#include "LevelZones.h"
#include "QuestProperties.h"
#include "c_QuestClient.h"
#include "pointsofinterest.h"
#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// Private structures
//----------------------------------------------------------------------------
STR_DICT pTargetCursorEnumTbl[] =
{
	//{ "targeting center",	TCUR_DEFAULT },
	{ "",	TCUR_DEFAULT },
	{ "hand cursor",		TCUR_HAND },
	{ "talk cursor",		TCUR_TALK },
	{ "lock on",			TCUR_LOCK },
	{ "sniper reticle",		TCUR_SNIPER },

	{ NULL,					-1		},
};

//----------------------------------------------------------------------------
// Forwards
//----------------------------------------------------------------------------
int UIGunModScreenOnSetGunModItem(
	UI_COMPONENT* component);

//----------------------------------------------------------------------------
// UI Component methods
//----------------------------------------------------------------------------
UI_HUD_TARGETING::UI_HUD_TARGETING(void)
: UI_COMPONENT()
, pTargetingCenter(NULL)
, eCursor(TCUR_DEFAULT)
, bShowBrackets(FALSE)
, m_bForceUpdate(FALSE)
{
	memset(ppCorner, 0, sizeof(UI_COMPONENT*) * arrsize(ppCorner));
	memset(pCursorFrame, 0, sizeof(UI_TEXTURE_FRAME*) * NUM_TARGET_CURSORS);
	memset(m_pAccuracyFrame, 0, sizeof(UI_TEXTURE_FRAME*) * UIALIGN_NUM);
	memset(m_dwAccuracyColor, 0, sizeof(DWORD) * arrsize(m_dwAccuracyColor));
}

UI_GUNMOD::UI_GUNMOD(void)
: UI_COMPONENT()
, m_pIconTexture(NULL)
, m_nItemSwitchSound(0)
{
	memset(m_ppModLocFrame, 0, sizeof(UI_TEXTURE_FRAME*) * MAX_MOD_LOC_FRAMES);
	memset(m_pnModLocFrameLoc, 0, sizeof(int) * MAX_MOD_LOC_FRAMES);
	memset(m_pnModLocString, 0, sizeof(int) * MAX_MOD_LOC_FRAMES);
}

UI_MODBOX::UI_MODBOX(void)
: UI_COMPONENT()
, m_nInvLocation(0)
, m_nInvGridIdx(0)
, m_fCellBorder(0.0f)
, m_pLitFrame(NULL)
, m_pIconFrame(NULL)
{
}

UI_SKILLPAGE::UI_SKILLPAGE(void)
: UI_COMPONENT ()
, m_nCurrentPage(0)
, m_nInvLoc(0)
, m_pLeftArrow(NULL)
, m_pRightArrow(NULL)
, m_pPageButtonLitFrame(NULL)
, m_pPageButtonUnlitFrame(NULL)
{
	memset(m_pPageButton, 0, sizeof(UI_BUTTONEX*) * arrsize(m_pPageButton));
}

UI_WEAPON_CONFIG::UI_WEAPON_CONFIG(void)
: UI_COMPONENT()
, m_pLeftSkillPanel(NULL)
, m_pRightSkillPanel(NULL)
, m_pWeaponPanel(NULL)
, m_pSelectedPanel(NULL)
, m_pKeyLabel(NULL)
, m_nHotkeyTag(0)
, m_nInvLocation(0)
, m_fDoubleWeaponFrameW(0.0f)
, m_fDoubleWeaponFrameH(0.0f)
, m_pFrame1Weapon(NULL)
, m_pFrame2Weapon(NULL)
, m_pWeaponLitFrame(NULL)
, m_pSkillLitFrame(NULL)
, m_pSelectedFrame(NULL)
, m_pDoubleSelectedFrame(NULL)
, m_pWeaponBkgdFrameR(NULL)
, m_pWeaponBkgdFrameL(NULL)
, m_pWeaponBkgdFrame2(NULL)
{
}

UI_WORLDMAP_LEVEL::UI_WORLDMAP_LEVEL(void)
: m_nLevelID(0)
, m_nRow(0)
, m_nCol(0)
, m_nLabelPosition(0)
, m_pFrame(NULL)
, m_pFrameHidden(NULL)
{
	memset(m_ppConnections, 0, sizeof(UI_WORLDMAP_LEVEL*) * MAX_WORLDMAP_LEVEL_CONNECTIONS);
}

UI_WORLD_MAP::UI_WORLD_MAP(void)
: UI_COMPONENT()
, m_pLevels(NULL)
, m_pVertConnector(NULL)
, m_pHorizConnector(NULL)
, m_pNESWConnector(NULL)
, m_pNWSEConnector(NULL)
, m_pYouAreHere(NULL)
, m_pPointOfInterest(NULL)
, m_fColWidth(0.0f)
, m_fRowHeight(0.0f)
, m_fDiagonalAdjustmentX(0.0f)
, m_fDiagonalAdjustmentY(0.0f)
, m_pLevelsPanel(NULL)
, m_pConnectionsPanel(NULL)
, m_pIconsPanel(NULL)
, m_dwConnectionColor(0)
, m_dwStationConnectionColor(0)
, m_pNextStoryQuestIcon(NULL)
, m_fOriginalMapWidth(0.0f)
, m_fOriginalMapHeight(0.0f)
, m_fIconOffsetX(0.0f)
, m_fIconOffsetY(0.0f)
, m_fIconScale(0.0f)
, m_pTownIcon(NULL)
, m_pDungeonIcon(NULL)
, m_pCaveIcon(NULL)
, m_pCryptIcon(NULL)
, m_pCampIcon(NULL)
, m_pTowerIcon(NULL)
, m_pChurchIcon(NULL)
, m_pCitadelIcon(NULL)
, m_pHengeIcon(NULL)
, m_pFarmIcon(NULL)
, m_pShrineIcon(NULL)
, m_pForestIcon(NULL)
, m_pPartyIsHere(NULL)
, m_pNewArea(NULL)
, m_pLineFrame(NULL)
{
}

UI_TRACKER::UI_TRACKER(void)
: UI_COMPONENT()
, m_pSweeperFrame(NULL)
, m_pCrosshairsFrame(NULL)
, m_pOverlayFrame(NULL)
, m_pRingsFrame(NULL)
, m_fScale(0.0f)
, m_nSweepPeriodMSecs(0)
, m_nRingsPeriodMSecs(0)
, m_nPingPeriodMSecs(0)
, m_fPingDistanceRads(0.0f)
, m_fRadius(0.0f)
, m_bTracking(FALSE)
{
	memset(m_pTargetFrame, 0, sizeof(UI_TEXTURE_FRAME*) * MAX_TARGET_FRAMES);
	memset(m_nTargetID, 0, sizeof(int) * MAX_TARGETS);
	memset(m_vTargetPos, 0, sizeof(VECTOR) * MAX_TARGETS);
	memset(m_nTargetFrameType, 0, sizeof(int) * MAX_TARGETS);
	memset(m_dwTargetColor, 0, sizeof(DWORD) * MAX_TARGETS);
}


//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------



UI_MSG_RETVAL ShiftSkillIconOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!component)
		return UIMSG_RET_NOT_HANDLED;

	BOOL bDrawingSkill = FALSE;
	BOOL bDrawingItem = FALSE;

	if (UIComponentGetVisible(component))
	{
		UIComponentRemoveAllElements(component);
		UI_COMPONENT *pSkillPanel = UIComponentFindChildByName(component, "skill icon panel");
		if (pSkillPanel)
		{
			UIComponentRemoveAllElements(pSkillPanel);
			pSkillPanel->m_dwData = INVALID_ID;

			int nSkillID = c_SkillsFindActivatorSkill( SKILL_ACTIVATOR_KEY_SHIFT );
			if (nSkillID != INVALID_ID)
			{
				UNIT *pFocusUnit = UIGetControlUnit();
				SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( AppGetCltGame(), DATATABLE_SKILLS, nSkillID );
				int nSkillLevel = SkillGetLevel( pFocusUnit, nSkillID );
				int nPowerCost = SkillGetPowerCost( pFocusUnit, pSkillData, nSkillLevel );
				if ( nPowerCost == 0 || UnitGetStat( pFocusUnit, STATS_POWER_CUR ) >= nPowerCost)
				{
					UI_RECT rect = UIComponentGetRect(pSkillPanel);
					UI_POSITION pos(pSkillPanel->m_fWidth / 2.0f, pSkillPanel->m_fHeight / 2.0f);
					BYTE byAlpha = (BYTE)(pSkillPanel->m_dwColor >> 24);
					UI_DRAWSKILLICON_PARAMS tParams(pSkillPanel, rect, pos, nSkillID, NULL, NULL, NULL, NULL, byAlpha);
					tParams.fSkillIconWidth = pSkillPanel->m_fWidth / UIGetAspectRatio();	// the width has already been adjusted for ar, so we have to adj it back 'cause the draw function's gonna adjust it again.
					tParams.fSkillIconHeight = pSkillPanel->m_fHeight;
					UIDrawSkillIcon(tParams);
					pSkillPanel->m_dwData = pSkillData->nId;
					bDrawingSkill = TRUE;
				}
			}
		}

		UI_COMPONENT *pItemPanel = UIComponentFindChildByName(component, "item icon panel");
		if (pItemPanel)
		{
			UIComponentRemoveAllElements(pItemPanel);
			pItemPanel->m_dwData = INVALID_ID;

			UNIT *pUseUnit = c_SkillsFindActivatorItem( SKILL_ACTIVATOR_KEY_POTION );

			if (pUseUnit)
			{
				//UI_TEXTURE_FRAME *pBackground = UIGetStdSkillIconBackground();
				//if (pBackground)
				//{
				//	UI_RECT rect = UIComponentGetRect(pItemPanel);
				//	UI_POSITION pos(pBackground->m_fWidth / 2.0f, pBackground->m_fHeight / 2.0f);
				//	BYTE byAlpha = (BYTE)(pItemPanel->m_dwColor >> 24);
				//	UIDrawSkillIcon(pItemPanel, rect, pos, INVALID_ID, pBackground, NULL, NULL, FALSE, FALSE, byAlpha);
				//}
				UIComponentAddItemGFXElement(pItemPanel, pUseUnit, 
					UI_RECT(0.0f, 0.0f, pItemPanel->m_fWidth, pItemPanel->m_fHeight), NULL, NULL);
				pItemPanel->m_dwData = pUseUnit->unitid;
				bDrawingItem = TRUE;
			}
		}
	}

	UI_COMPONENT *panel = UIComponentFindChildByName(component, "use skill bkgd");
	if (panel)
		UIComponentSetVisible(panel, bDrawingSkill);

	panel = UIComponentFindChildByName(component, "use item bkgd");
	if (panel)
		UIComponentSetVisible(panel, bDrawingItem);

	// Using the component's data variable here, because the anim ticket will be used when it slides in and out
	if (component->m_dwData != INVALID_ID)
	{
        DWORD d = (DWORD)component->m_dwData;
		CSchedulerCancelEvent(d);
	}
	component->m_dwData = CSchedulerRegisterMessage(AppCommonGetCurTime() + 500, component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UISkillIconPanelOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetVisible(component) || !UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nSkillID = (int)component->m_dwData;
	if (nSkillID == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UISetHoverTextSkill(component, UIGetControlUnit(), nSkillID);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UISkillIconPanelOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UISetHoverUnit(INVALID_ID, INVALID_ID);                                            
	UIClearHoverText();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIItemIconPanelOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nUnitID = (int)component->m_dwData;
	if (nUnitID == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pUnit = UnitGetById(AppGetCltGame(), nUnitID);
	if (pUnit == NULL)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UISetHoverTextItem(component, pUnit);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIItemIconPanelOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UISetHoverUnit(INVALID_ID, INVALID_ID);
	UIClearHoverText();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sUIModUnitSetMipBonusEnabled( BOOL bEnabled )
{
	GAME * pGame = AppGetCltGame();
	{
		UNIT * item = UnitGetById( pGame, UIGetModUnit() );
		if ( item )
		{
			int nWardrobe = c_AppearanceGetWardrobe( 
				c_UnitGetModelIdInventoryInspect( item ) );
			if ( nWardrobe != INVALID_ID )
			{
				c_WardrobeSetMipBonusEnabled( nWardrobe, bEnabled );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIGunModScreenFreeModel()
{
	UNITID idModUnit = UIGetModUnit();
	UNIT * pModUnit = UnitGetById(AppGetCltGame(), idModUnit);
	if(pModUnit)
	{
		UnitGfxInventoryInspectFree(pModUnit);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UIPlaceMod(
	UNIT* itemtomod,
	UNIT* itemtoplace,
	BOOL bShowScreen /*= TRUE*/)
{
	if (bShowScreen)
	{
		UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_GUNMODSCREEN);
		if (!component)
		{
			return;
		}
		BOOL bActivate = (UIGetModUnit() != UnitGetId(itemtomod));
		UIComponentActivate(component, bActivate);

		// Re-enable mip bonus of old mod unit before switching to
		// another mod unit.
		sUIModUnitSetMipBonusEnabled( TRUE );

		if (bActivate)
		{
			UISetHoverUnit(INVALID_ID, UIComponentGetId(component));

			sUIGunModScreenFreeModel();

			UISetModUnit(UnitGetId(itemtomod));
			UIGunModScreenOnSetGunModItem(component);

			// Disable mip bonus when viewing object in detail.
			sUIModUnitSetMipBonusEnabled( FALSE );
		}
		else if (!itemtoplace)
		{
			if (component->m_eState != UISTATE_ACTIVE)		// if the gunmod screen is moving, leave it alone
			{
				return;
			}

			UIComponentHandleUIMessage(component, UIMSG_INACTIVATE, 0, 0);
		}
	}

	if (!itemtoplace || !itemtomod)
	{
		return;
	}

	if (ItemBelongsToAnyMerchant(itemtomod))
	{
		return;
	}

	int location, x, y;
	BOOL bHasSpace = ItemGetEquipLocation(itemtomod, itemtoplace, &location, &x, &y);
	if (!bHasSpace)
	{
		return;
	}

	if (!UICheckRequirementsWithFailMessage(itemtomod, itemtoplace, NULL, location, NULL))
	{
		return;
	}

	if (UnitInventoryTest(itemtomod, itemtoplace, location, x))
	{
		MSG_CCMD_ITEMINVPUT msg;
		msg.idContainer = UnitGetId(itemtomod);
		msg.idItem = UnitGetId(itemtoplace);
		msg.bLocation = (BYTE)location;
		msg.bX = (BYTE)x;
		msg.bY = 0;
		c_SendMessage(CCMD_ITEMINVPUT, &msg);
//		UISetCursorUnit(INVALID_ID, TRUE);
		UnitWaitingForInventoryMove(itemtoplace, TRUE);
		UIPlayItemPutdownSound(itemtoplace, itemtomod, location);
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* UIModBoxGetContainer(
	void)
{
	if (UIGetModUnit() == INVALID_ID)
	{
		return NULL;
	}
	UNIT* container = UnitGetById(AppGetCltGame(), UIGetModUnit());
	return container;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModBoxOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	//UNIT* container = UnitGetById(AppGetCltGame(), (UNITID)wParam);
	//if (!container)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	//if (!pFocusUnit ||
	//	(UnitGetId(container) != UnitGetId(pFocusUnit) &&
	//	UnitGetId(container) != UnitGetOwnerId(pFocusUnit)))
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//container = pFocusUnit;

	UNIT* container = UIComponentGetFocusUnit(component);
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	ASSERT(UnitIsA(container, UNITTYPE_ITEM));

	UI_MODBOX* modbox = UICastToModBox(component);
	//if (!UIComponentGetVisible(modbox) || !UIComponentGetActive(modbox->m_pParent))
	//{
	//	return 0;
	//}
	//int location = (int)lParam;
	//if (location == -1)
	//{
	//	location = modbox->m_nInvLocation;
	//}
	//if (location != modbox->m_nInvLocation)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	int location = modbox->m_nInvLocation;

	UIComponentRemoveAllElements(modbox);

	UIX_TEXTURE* texture = UIComponentGetTexture(modbox);
	ASSERT_RETVAL(texture, UIMSG_RET_NOT_HANDLED);

	if (modbox->m_pFrame)
	{
		UIComponentAddElement(modbox, texture, modbox->m_pFrame, UI_POSITION(0.0f, 0.0f));
	}

	UNIT* item = UnitInventoryGetByLocationAndIndex(container, location, modbox->m_nInvGridIdx);
	if (!item)
	{
		// put the little symbol for the mod type in the box
		if (modbox->m_pIconFrame && modbox->m_pFirstChild)
		{
			UI_GUNMOD * gunmod = UICastToGunMod(component->m_pParent);
			UIComponentAddElement(modbox->m_pFirstChild, gunmod->m_pIconTexture, modbox->m_pIconFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_HOTPINK, NULL, FALSE, 1.0f, 1.0f, &UI_SIZE(modbox->m_fWidth, modbox->m_fHeight));
		}

		if (UIGetCursorUnit() == INVALID_ID)
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		UNIT* pCursorItem = UnitGetById(AppGetCltGame(), UIGetCursorUnit());
		if (!pCursorItem)
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		if (!UnitInventoryTest(container, pCursorItem, location, modbox->m_nInvGridIdx))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		BOOL bMeetsRequirements = InventoryCheckStatRequirements(container, pCursorItem, NULL, NULL, location);
		if (modbox->m_pLitFrame)
		{
			UIComponentAddElement(component, texture, modbox->m_pLitFrame, UI_POSITION(0.0f, 0.0f), bMeetsRequirements ? GFXCOLOR_GREEN : GFXCOLOR_RED);
		}
		else
		{
			UIComponentAddBoxElement(modbox, modbox->m_fCellBorder, modbox->m_fCellBorder, 
				modbox->m_fWidth - modbox->m_fCellBorder, modbox->m_fHeight - modbox->m_fCellBorder, NULL, bMeetsRequirements ? UI_HILITE_GREEN : GFXCOLOR_RED, 24);
		}
		return UIMSG_RET_HANDLED;
	}

	if (modbox->m_pLitFrame)
	{
		DWORD dwColor = GFXCOLOR_WHITE;
		int nColor = UIInvGetItemGridBackgroundColor(item);
		if (nColor > 0)
		{
			dwColor = GetFontColor(nColor);
		}
		UIComponentAddElement(component, texture, modbox->m_pLitFrame, UI_POSITION(0.0f, 0.0f), dwColor);
	}

	UI_RECT gridrect;
	gridrect.m_fX1 = 0.0f;
	gridrect.m_fY1 = 0.0f;
	gridrect.m_fX2 = (modbox->m_fWidth - modbox->m_fCellBorder);
	gridrect.m_fY2 = (modbox->m_fHeight - modbox->m_fCellBorder);

	if (modbox->m_pFirstChild)
	{
		UI_GFXELEMENT* element = UIComponentAddModelElement(modbox->m_pFirstChild, UnitGetAppearanceDefIdForCamera( item ), 
			c_UnitGetModelIdInventoryInspect( item ), gridrect, 0.0f);

		if (UnitGetId(item) == UIGetCursorUnit())
		{
			element->m_dwColor = UI_ITEM_GRAY;
		}
	}

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModBoxOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component)
	{
		UISetHoverUnit(INVALID_ID, INVALID_ID);
		UIClearHoverText();
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModBoxOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// the repaint should be done by the parent panel
	return UIMSG_RET_NOT_HANDLED;
	
	//UNIT* container = UnitGetById(AppGetCltGame(), (UNITID)wParam);
	//if (!container)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}
	//UNIT* pFocus = UIComponentGetFocusUnit(component);
	//if (!pFocus)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	//if (container != pFocus &&
	//	container != UnitGetUltimateContainer(pFocus))
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//UI_MODBOX *pModBox = UICastToModBox(component);

	//int location = (int)lParam;
	//if (location != -1 && location != pModBox->m_nInvLocation && location != nCursorLocation)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}
	//
	//UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	//return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModBoxOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MODBOX* modbox = UICastToModBox(component);
	if (!UIComponentGetActive(modbox))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x, y;
	UI_POSITION pos;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* container = UIModBoxGetContainer();
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int location = modbox->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* item = UnitInventoryGetByLocationAndIndex(container, location, modbox->m_nInvGridIdx);
	if (!item)
	{
		UISetHoverUnit(INVALID_ID, UIComponentGetId(modbox));
		if( modbox->m_szTooltipText )
		{
			UISetSimpleHoverText(modbox, modbox->m_szTooltipText);
		}
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	UISetHoverTextItem(
		&UI_RECT(pos.m_fX, 
				 pos.m_fY, 
				 pos.m_fX + component->m_fWidth, 
				 pos.m_fY + component->m_fHeight), 
		item);
	UISetHoverUnit(UnitGetId(item), UIComponentGetId(modbox));
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIModBoxOnLButtonDownNoCursorUnit(
	UI_MODBOX* modbox)
{

// no longer allowing the player to pick mods up out of items	

	//UNIT* container = UIModBoxGetContainer();
	//if (!container)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//int location = modbox->m_nInvLocation;
	//INVLOC_HEADER info;
	//if (!UnitInventoryGetLocInfo(container, location, &info))
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//float x, y;
	//UIGetCursorPosition(&x, &y);

	//UI_POSITION pos;
	//if (!UIComponentCheckBounds(modbox, x, y, &pos))
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//UNIT* item = UnitInventoryGetByLocationAndIndex(container, location, modbox->m_nInvGridIdx);
	//if (!item)
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	//UISetCursorUnit(UnitGetId(item), TRUE);
	//UIPlayItemPickupSound(item);
	//int nSoundId = GlobalIndexGet(GI_SOUND_WEAPONCONFIG_1);
	int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
	if (nSoundId != INVALID_LINK)
	{
		c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
	}


	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIModBoxOnLButtonDownWithCursorUnit(
	UI_MODBOX* modbox)
{
	UNITID idCursorUnit = UIGetCursorUnit();
	UNIT* unitCursor = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!unitCursor)
	{
		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* container = UIModBoxGetContainer();
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x, y;
	UIGetCursorPosition(&x, &y);

	int location = modbox->m_nInvLocation;
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(container, location, &info))
	{
		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_POSITION pos;
	if (!UIComponentCheckBounds(modbox, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (ItemBelongsToAnyMerchant(container))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (ItemIsInSharedStash(container))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	UNIT* item = UnitInventoryGetByLocationAndIndex(container, location, modbox->m_nInvGridIdx);
	if (!UICheckRequirementsWithFailMessage(container, unitCursor, item, location, modbox))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (!item)
	{
		if (UnitInventoryTest(container, unitCursor, location, modbox->m_nInvGridIdx))
		{
			if (ItemBelongsToAnyMerchant(unitCursor))
			{
				// first we have to buy it
				int location = GlobalIndexGet( GI_INVENTORY_LOCATION_CURSOR );
				if (!UIDoMerchantAction(unitCursor, location, 0, 0))
				{
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}

			MSG_CCMD_ITEMINVPUT msg;
			msg.idContainer = UnitGetId(container);
			msg.idItem = idCursorUnit;
			msg.bLocation = (BYTE)location;
			msg.bX = (BYTE)modbox->m_nInvGridIdx;
			msg.bY = 0;
			c_SendMessage(CCMD_ITEMINVPUT, &msg);
			UnitWaitingForInventoryMove(unitCursor, TRUE);
//			UISetCursorUnit(INVALID_ID, TRUE);
			UIPlayItemPutdownSound(unitCursor, container, location);

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		else
		{
			return UIMSG_RET_HANDLED;
		}
	}
	if (item == unitCursor)
	{
		UIPlayItemPutdownSound(unitCursor, container, location);
		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_HANDLED;
	}

	// check if a swap is possible
	if (!UnitInventoryTestSwap(item, unitCursor))
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (ItemBelongsToAnyMerchant(unitCursor))
	{
		// first we have to buy it
		int location = GlobalIndexGet( GI_INVENTORY_LOCATION_CURSOR );
		if (!UIDoMerchantAction(unitCursor, location, 0, 0))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	c_InventoryTrySwap(unitCursor, item);
	UIPlayItemPutdownSound(unitCursor, container, location);
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModBoxOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{


	UI_MODBOX* modbox = UICastToModBox(component);

	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (UIGetCursorSkill() != INVALID_ID)
		return UIMSG_RET_HANDLED_END_PROCESS;
	if (UIGetCursorUnit() == INVALID_ID)
	{
		return sUIModBoxOnLButtonDownNoCursorUnit(modbox);
	}
	else
	{
		return sUIModBoxOnLButtonDownWithCursorUnit(modbox);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModImageOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);
	if (UIGetModUnit() == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* item = UnitGetById(AppGetCltGame(), UIGetModUnit());
	if (!item)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_PANELEX *panel = UICastToPanel(component);

	UIComponentAddModelElement(component, UnitGetAppearanceDefIdForCamera( item ), 
		c_UnitGetModelIdInventoryInspect( item ), UI_RECT(0.0f, 0.0f, component->m_fWidth, component->m_fHeight), panel->m_fModelRotation);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModScreenOnActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIComponentOnActivate(component, msg, wParam, lParam);
	if (ResultIsHandled(eResult))
	{
		UI_COMPONENT *pNPCConversation = UIComponentGetByEnum(UICOMP_NPC_CONVERSATION_DIALOG);
		if (pNPCConversation && UIComponentGetVisible(pNPCConversation))
		{
			pNPCConversation->m_Position.m_fX = component->m_Position.m_fX - pNPCConversation->m_fWidth;
		}
	}

	return eResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModScreenOnInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component->m_eState == UISTATE_INACTIVE ||
		component->m_eState == UISTATE_INACTIVATING)
	{
		return UIMSG_RET_HANDLED;
	}

	UISetHoverUnit(INVALID_ID, INVALID_ID);

	UNITID idModUnit = UIGetModUnit();
	UNIT * pModUnit = UnitGetById(AppGetCltGame(), idModUnit);
	if(pModUnit)
	{
		UnitGfxInventoryInspectFree(pModUnit);
	}

	UI_GUNMOD *pGunMod = UICastToGunMod(component);
	ASSERT_RETVAL(pGunMod, UIMSG_RET_NOT_HANDLED);

	// Re-enable mip bonus of mod unit before closing screen.
	sUIModUnitSetMipBonusEnabled( TRUE );
	UISetModUnit(INVALID_ID);
	
	// BSP - um... not blatant self promotion... Brennan Scott Plunkett ... although now that I think about it...
	// not sure why these were here, but they cause problems now.  Delete after a while if they're still not needed.
	//UISendMessage(WM_INVENTORYCHANGE, UnitGetId(UIGetControlUnit()), -1);	//force redraw of the inventory
	//UISendMessage(WM_PAINT, 0, 0);		// this is so the inv grids can repaint themselves based on what the mod item is.

	UI_MSG_RETVAL eReturn = UIComponentOnInactivate(component, FALSE, 0, 0);

	if (ResultIsHandled(eReturn))
	{
		UI_COMPONENT *pNPCConversation = UIComponentGetByEnum(UICOMP_NPC_CONVERSATION_DIALOG);
		if (pNPCConversation && UIComponentGetVisible(pNPCConversation))
		{
			pNPCConversation->m_Position = pNPCConversation->m_ActivePos;
		}

		CSchedulerRegisterMessage(AppCommonGetCurTime() + component->m_dwAnimDuration, UIComponentGetByEnum(UICOMP_INVENTORYSCREEN), UIMSG_PAINT, 0, 0);
	}

	return eReturn;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModScreenOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_GUNMOD* gunmod = UICastToGunMod(component);
	ASSERT_RETVAL(gunmod, UIMSG_RET_NOT_HANDLED);

	// make sure the item's still around
	UNIT* pGunModItem = UnitGetById(AppGetCltGame(), UIGetModUnit());
	if (!pGunModItem || UnitGetContainer(pGunModItem) == NULL)
	{
		UIComponentActivate(component, FALSE);
		//UIComponentHandleUIMessage(component, UIMSG_INACTIVATE, 0, 0);	// need to send the message so we play the deactivate sound
		//UIGunModScreenOnInactivate(component, UIMSG_INACTIVATE, 0, 0);
		return UIMSG_RET_NOT_HANDLED;
	}

	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	int location = (int)lParam;
	UNIT* container = UnitGetById(AppGetCltGame(), (UNITID)wParam);
	if (container == NULL ||
		container == pGunModItem ||
		UnitGetUltimateContainer(container) == pGunModItem ||
		location == -1 ||
		location == nCursorLocation)
	{
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUIGunModOnLButtonDownWithCursorUnit(
	UI_GUNMOD* gunmod)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	if (!UIComponentCheckBounds(gunmod, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNITID idCursorUnit = UIGetCursorUnit();
	UNIT* unitCursor = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!unitCursor)
	{
		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* container = UnitGetById(AppGetCltGame(), UIGetModUnit());
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (ItemIsInSharedStash(container))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!(UnitIsA(unitCursor, UNITTYPE_MOD) && UnitGetInventoryType(container) > 0))
	{
		return UIMSG_RET_HANDLED;
	}

	UIPlaceMod(container, unitCursor, FALSE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (UIGetCursorSkill() != INVALID_ID)
		return UIMSG_RET_HANDLED_END_PROCESS;

	UI_GUNMOD* gunmod = UICastToGunMod(component);
	ASSERT_RETVAL(gunmod, UIMSG_RET_NOT_HANDLED);

	if (UIGetCursorUnit() == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	else
	{
		return sUIGunModOnLButtonDownWithCursorUnit(gunmod);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModTooltipOnSetHoverUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (component->m_pParent && UIComponentGetActive(component->m_pParent))
	{
		UNIT* item = UnitGetById(AppGetCltGame(), UNITID(wParam));
		UIComponentSetVisible(component, item == NULL );
		return UIMSG_RET_HANDLED;
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIGunModScreenOnSetGunModItem(
	UI_COMPONENT* component)
{
	UI_GUNMOD* gunmod = UICastToGunMod(component);
	ASSERT_RETZERO(gunmod);

	if (UIGetModUnit() == INVALID_ID)
	{
		UIComponentSetFocusUnit(component, INVALID_ID);
		return 0;
	}
	GAME * pGame = AppGetCltGame();
	UNIT* item = UnitGetById( pGame, UIGetModUnit());
	if (!item)
	{
		UIComponentSetFocusUnit(component, INVALID_ID);
		return 0;
	}

	UnitDataLoad(pGame, DATATABLE_ITEMS, UnitGetClass(item), UDLT_INSPECT);
	UIItemInitInventoryGfx(item, TRUE, TRUE);

	UIComponentSetFocusUnit(component, UIGetModUnit());

	UI_COMPONENT* gunimage = UIComponentFindChildByName(component, "gun image");
	if (gunimage)
	{
		gunimage->m_dwParam = 0;
		UIComponentHandleUIMessage(gunimage, UIMSG_PAINT, 0, 0);
		//UIGunModScreenOnSetImage(gunimage);
	}

	// set all children to inactive
	UI_COMPONENT* child = gunmod->m_pFirstChild;
	while (child)
	{
		if (child->m_eComponentType == UITYPE_MODBOX ||
			child->m_eComponentType == UITYPE_LABEL )
		{
			if (PStrICmp(child->m_szName, "MODIFICATION PANEL") != 0)
				UIComponentSetVisible(child, FALSE);
		}
		child = child->m_pNextSibling;
	}

	BOOL bGambler = ItemBelongsToAnyMerchant( item ) && UnitIsGambler( ItemGetMerchant(item) );

	int curbox = 1;
	PARAM dwParam = 0;
	while (!bGambler)
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
				char boxname[256];
				PStrPrintf(boxname, 256, "modbox %d", curbox);
				curbox++;
				UI_COMPONENT* child = UIComponentFindChildByName(component, boxname);
				if (!child)
				{
					return 0;
				}
				UI_MODBOX* modbox = UICastToModBox(child);
				if (!modbox)
				{
					return 0;
				}
				//modbox->m_Position.m_fX = config->m_pPositions[jj].m_fX;
				//modbox->m_Position.m_fY = config->m_pPositions[jj].m_fY;
				modbox->m_nInvLocation = location;
				modbox->m_nInvGridIdx = jj;
				modbox->m_pIconFrame = NULL;
				if (location != INVLOC_NONE)
				{
					for (int k=0; k < UI_GUNMOD::MAX_MOD_LOC_FRAMES; k++)
					{
						if (gunmod->m_pnModLocFrameLoc[k] == location)
						{
							modbox->m_pIconFrame = gunmod->m_ppModLocFrame[k];
							if (gunmod->m_pnModLocString[k] != INVALID_ID)
							{
								if( !modbox->m_szTooltipText )
								{
									modbox->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
								}

								PStrCopy(modbox->m_szTooltipText, StringTableGetStringByIndex(gunmod->m_pnModLocString[k]), UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
							}
						}
					}
				}
				UIComponentSetActive(modbox, TRUE);
//				UIComponentHandleUIMessage(modbox, UIMSG_PAINT, 0, 0);
			}
		}
		if (count < 64)
		{
			break;
		}
		dwParam = stats_entry[count-1].GetParam() + 1;
	}

	UISendMessage(WM_PAINT, 0, 0);		// this is so the inv grids can repaint themselves based on what the mod item is.
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGunModScreenOnCancelClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (!component->m_pParent)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

//	UISwitchComponents(UICOMP_GUNMODSCREEN, UICOMP_PAPERDOLL);
//	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_GUNMODSCREEN), UIMSG_INACTIVATE, 0, 0);
	UIComponentActivate(UICOMP_GUNMODSCREEN, FALSE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


/*
Old - before Tugboat merge
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void TransformUnitSpaceToScreenSpace(
	UNIT* unit,
	int* x,
	int* y)
{
	ASSERT_RETURN(unit);

	// transform unit position to screen space
	MATRIX mOut;

	e_GetWorldViewProjMatrix( &mOut );

	VECTOR vLoc = UnitGetAimPoint( unit );

	VECTOR4 vNewLoc;
	MatrixMultiply( &vNewLoc, &vLoc, &mOut );
	vNewLoc.fX = ((vNewLoc.fX / vNewLoc.fW + 1.0f) / 2.0f) * UI_DEFAULT_WIDTH;
	vNewLoc.fY = ((-vNewLoc.fY / vNewLoc.fW + 1.0f) / 2.0f) * UI_DEFAULT_HEIGHT;
	*x = (int)(FLOOR(vNewLoc.fX + 0.5f));
	*y = (int)(FLOOR(vNewLoc.fY + 0.5f));
}*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGetCrosshairsAccuracy(
	int nHand,
	float &fAccX, 
	float &fAccY)
{
	GAME * pGame = AppGetCltGame();
	if ( !pGame || ! IS_CLIENT( pGame ) )
		return FALSE;

	UNIT * pControlUnit = GameGetControlUnit( pGame );
	if ( ! pControlUnit )
		return FALSE;

	if ( UnitIsInTown( pControlUnit ) )
		return FALSE;

	int nWardrobe = UnitGetWardrobe( pControlUnit );
	if ( nWardrobe == INVALID_ID )
		return FALSE;

	UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, nHand );
	if ( ! pWeapon )
		return FALSE;

	if ( UnitGetContainer( pWeapon ) != pControlUnit )
		return FALSE; // sometimes we are between levels or something and this happens

	int nSkill = ItemGetPrimarySkill( pWeapon );
	if ( nSkill == INVALID_ID )
		return FALSE;

	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if (! pSkillData )
		return FALSE;

	if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_DISPLAY_FIRING_ERROR ) )
		return FALSE;

	const UNIT_DATA * pMissileData = NULL;
	if ( pSkillData->pnMissileIds[ 0 ] != INVALID_ID )
		pMissileData = MissileGetData( NULL, pSkillData->pnMissileIds[ 0 ] );

	float fHorizSpread = 0.0f;
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_HELLGATE_EXTRA_MISSILE_HORIZ_SPREAD ) ||
		UnitGetStat( pControlUnit, STATS_MULTI_BULLET_COUNT ) )
		fHorizSpread = HELLGATE_EXTRA_MISSILE_HORIZ_SPREAD;

	float fVertRange = 0.0f;
	float fHorizRange = 0.0f;
	if ( pMissileData )
	{
		SkillGetAccuracy( pControlUnit, pWeapon, nSkill, pMissileData, fHorizSpread, fVertRange, fHorizRange );
	}

	int nHorizFOVDegrees = c_CameraGetHorizontalFovDegrees( c_CameraGetViewType() );
	ASSERT_RETFALSE( nHorizFOVDegrees > 0 );
	float fRadians = (105.0f / (float)nHorizFOVDegrees) * PI_OVER_TWO;

	float fAspect = e_GetWindowAspect();

	fAccX = fHorizRange * fRadians / fAspect;
	fAccY = fVertRange * fRadians;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define CROSS_ACC_MAGIC	0x7afc00
#define NUM_ACCURACY_INDICATORS 2
#define ACCURACY_LERP_SPEED 0.6f
void sAdjustAccuracyElements(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_HUD_TARGETING *pTargeting = (UI_HUD_TARGETING *)data.m_Data1;

	ASSERT_RETURN(pTargeting);

	// lerp between current and previous values in order to smooth it out
	static float fAccPrevX[NUM_ACCURACY_INDICATORS] = {0.0f, 0.0f};
	static float fAccPrevY[NUM_ACCURACY_INDICATORS] = {0.0f, 0.0f};

	float fAccCurrX[NUM_ACCURACY_INDICATORS] = {0.0f, 0.0f};
	float fAccCurrY[NUM_ACCURACY_INDICATORS] = {0.0f, 0.0f};
	BOOL bEnabled[NUM_ACCURACY_INDICATORS] = {FALSE, FALSE};
	for (int i=0; i<NUM_ACCURACY_INDICATORS; i++)
	{
		bEnabled[i] = sGetCrosshairsAccuracy(i, fAccCurrX[i], fAccCurrY[i]);
		if ( bEnabled[i] )
		{
			fAccCurrX[ i ] = ACCURACY_LERP_SPEED * fAccPrevX[ i ] + (1.0f - ACCURACY_LERP_SPEED) * fAccCurrX[ i ];
			fAccCurrY[ i ] = ACCURACY_LERP_SPEED * fAccPrevY[ i ] + (1.0f - ACCURACY_LERP_SPEED) * fAccCurrY[ i ];
		} else {
			fAccCurrX[ i ] = 0.0f;
			fAccCurrY[ i ] = 0.0f;
		}
		fAccPrevX[ i ] = fAccCurrX[ i ];
		fAccPrevY[ i ] = fAccCurrY[ i ];
	}

	float fCenterX = pTargeting->m_fWidth / 2.0f;
	float fCenterY = pTargeting->m_fHeight / 2.0f;

	UI_GFXELEMENT *pElement = pTargeting->m_pGfxElementFirst;
	while (pElement)
	{
		UI_POSITION pos(fCenterX, fCenterY);
		if ((pElement->m_qwData & 0xffff00) == CROSS_ACC_MAGIC)
		{
			int nAlign = (int)((pElement->m_qwData & 0xf0) >> 4);
			int nHand = (int)(pElement->m_qwData & 0xf);

			if ( bEnabled[nHand] )
			{
				if (e_UIAlignIsTop(nAlign))		pos.m_fY -= (fCenterY * fAccCurrY[nHand]);
				if (e_UIAlignIsRight(nAlign))	pos.m_fX += (fCenterX * fAccCurrX[nHand]);
				if (e_UIAlignIsBottom(nAlign))	pos.m_fY += (fCenterY * fAccCurrY[nHand]);
				if (e_UIAlignIsLeft(nAlign))	pos.m_fX -= (fCenterX * fAccCurrX[nHand]);

				pElement->m_Position = pos;
			}
		}
		pElement = pElement->m_pNextElement;
	}

	UISetNeedToRender(pTargeting);

	pTargeting->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(sAdjustAccuracyElements, CEVENT_DATA((DWORD_PTR)pTargeting));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICrossHairsShow(
	BOOL bShow)
{
	//UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_TARGETCOMPLEX);
	//if (!component)
	//{
	//	return;
	//}

	//if (bShow)
	//	UIComponentSetActive(component, TRUE);
	//else
	//	UIComponentSetVisible(component, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITargetingOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_HUD_TARGETING *pTargeting = UICastToHUDTargeting(component);
	pTargeting->m_bForceUpdate = TRUE;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sShowCrosshairs(
	void)
{
	static const UI_COMPONENT_ENUM peComponents[] = 
	{
		UICOMP_RESTART,
		UICOMP_PAPERDOLL,
		UICOMP_SKILLMAP,
		UICOMP_PLAYERPACK,
		UICOMP_WORLD_MAP,
		UICOMP_RTS_MINIONS,
		UICOMP_GAMEMENU,
		UICOMP_KIOSK,
		UICOMP_AUTOMAP_BIG,
		UICOMP_QUEST_PANEL,
		UICOMP_ACHIEVEMENTS,
	};

	APP_STATE eState = AppGetState();
	if (eState != APP_STATE_IN_GAME)
	{
		return FALSE;
	}

	for (int i=0; i< arrsize(peComponents); i++)
	{
		if (UIComponentGetVisibleByEnum(peComponents[i]))
			return FALSE;
	}

	// if the control unit has a certain state on it, we don't show the targeting
	UNIT *pControlUnit = UIGetControlUnit();
	if (pControlUnit)
	{
		if (UnitHasState( AppGetCltGame(), pControlUnit, STATE_HIDE_CROSSHAIRS ))
		{
			return FALSE;
		}
	}
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITargetChangeCursor(
	UNIT* target,
	TARGET_CURSOR eCursor,
	DWORD dwColor,
	BOOL bShowBrackets,
	DWORD dwColor2 /*= GFXCOLOR_HOTPINK*/)
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_TARGETCOMPLEX);
	if (!component)
	{
		return;
	}

	GAME* game = AppGetCltGame();
	if (!game || AppCommonGetDemoMode() )
	{
		UIComponentSetVisible(component, FALSE);
		return;
	}
	UNIT *pPlayer = GameGetControlUnit(game);

	if ((eCursor == TCUR_LOCK || eCursor == TCUR_DEFAULT) &&
		pPlayer &&
		UnitHasState(game, pPlayer, STATE_SNIPER))
	{
		eCursor = TCUR_SNIPER;
	}

	bShowBrackets &= g_UI.m_bShowTargetIndicator;

	UI_HUD_TARGETING *pTargeting = UICastToHUDTargeting(component);

	static BOOL bVisibleBefore = FALSE;
	if (!sShowCrosshairs() ||
		eCursor == TCUR_NONE)
	{
		UIComponentSetVisible(pTargeting, FALSE);
		bVisibleBefore = FALSE;
		return;
	}

	if (bVisibleBefore == FALSE)
	{
		pTargeting->m_bForceUpdate = TRUE;
		bVisibleBefore = TRUE;
	}

	if (!pTargeting->m_bForceUpdate &&
		target == NULL &&
		UIComponentGetFocusUnit(pTargeting) == target &&
		pTargeting->eCursor == eCursor &&
		pTargeting->m_dwColor == dwColor &&
		pTargeting->bShowBrackets == bShowBrackets)
	{
		// Nothing's changed.  Get out of here.
		return;
	}
	UIComponentSetFocusUnit(pTargeting, target ? UnitGetId(target) : INVALID_ID);
	pTargeting->eCursor = eCursor;
	pTargeting->m_dwColor = dwColor;
	pTargeting->bShowBrackets = bShowBrackets;
	pTargeting->m_bForceUpdate = FALSE;

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return;
	}
	pTargeting->pTargetingCenter->m_bVisible = pTargeting->m_bVisible;

	UIComponentRemoveAllElements(pTargeting);
	UIComponentRemoveAllElements(pTargeting->pTargetingCenter);

	if (IsUnitDeadOrDying(GameGetControlUnit(game)))
	{
		UIComponentSetVisible(pTargeting, FALSE);
		return;
	}

	UIComponentSetVisible(pTargeting, TRUE);
	if ((!target || UnitGetTargetType(target) == TARGET_BAD) &&
		eCursor != TCUR_SNIPER)
	{
#ifdef DRB_3RD_PERSON
		if ( c_CameraGetViewType() == VIEW_1STPERSON )
			UIComponentAddElement(pTargeting->pTargetingCenter, texture, pTargeting->pTargetingCenter->m_pFrame, UI_POSITION(), pTargeting->pTargetingCenter->m_dwColor);
#else
		UIComponentAddElement(pTargeting->pTargetingCenter, texture, pTargeting->pTargetingCenter->m_pFrame, UI_POSITION(), pTargeting->pTargetingCenter->m_dwColor);
#endif
	}

	// Draw the accuracy brackets
	if (pTargeting->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)		
		CSchedulerCancelEvent(pTargeting->m_tMainAnimInfo.m_dwAnimTicket);

	if (UITestFlag(UI_FLAG_SHOW_ACC_BRACKETS) &&
		(eCursor == TCUR_DEFAULT || eCursor == TCUR_LOCK || eCursor == TCUR_SNIPER))
	{
		for (int nHand = 0; nHand < NUM_ACCURACY_INDICATORS; nHand++)
		{
			float fAccCurrX;
			float fAccCurrY;
			if (sGetCrosshairsAccuracy(nHand, fAccCurrX, fAccCurrY))
			{
				for (int i=0; i < UIALIGN_NUM; i++)
				{
					if (pTargeting->m_pAccuracyFrame[i])
					{
						UI_GFXELEMENT *pElement = UIComponentAddElement(pTargeting, texture, pTargeting->m_pAccuracyFrame[i], UI_POSITION(), pTargeting->m_dwAccuracyColor[nHand], NULL, FALSE, 0.5f, 0.5f);
						pElement->m_qwData = CROSS_ACC_MAGIC | (i << 4) | nHand;
					}
				}
			}
		}

		//pTargeting->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(sAdjustAccuracyElements, CEVENT_DATA((DWORD_PTR)pTargeting));
		sAdjustAccuracyElements(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pTargeting), 0);
	}

	BOOL bBracketsExist = pTargeting->ppCorner[0] && pTargeting->ppCorner[1] && pTargeting->ppCorner[2] && pTargeting->ppCorner[3];

	if (target || eCursor == TCUR_SNIPER)
	{
		if (g_UI.m_bShowTargetIndicator && 
			(!UITestFlag(UI_FLAG_SHOW_TARGET_BRACKETS) || eCursor != TCUR_LOCK))
		{
			UI_TEXTURE_FRAME* frame = pTargeting->pCursorFrame[eCursor];
			if (frame)
			{
				UI_POSITION pos;
				float fScale = 1.0f;
				if (eCursor == TCUR_LOCK)
				{
					// put the indicator over the unit's head
					const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
					if (!pCameraInfo)
					{
						return;
					}
					VECTOR vEye = CameraInfoGetPosition( pCameraInfo );
					VECTOR vScreenPos;
					float fControlDistSq;
					float fCameraDistSq;
					UnitDisplayGetOverHeadPositions( 
						target,
						GameGetControlUnit(AppGetCltGame()),
						vEye,
						1.5f, 
						vScreenPos,
						fScale,
						fControlDistSq,
						fCameraDistSq);

					fScale = PIN(fScale * 4.0f, 0.2f, 2.0f);	
					fScale *= UnitGetLabelScale( target );				

					pos.m_fX = (vScreenPos.fX + 1.0f) * 0.5f * UIDefaultWidth();
					pos.m_fY = (-vScreenPos.fY + 1.0f) * 0.5f * UIDefaultHeight();

					pos.m_fX -= (frame->m_fWidth * fScale) / 2.0f ;
					pos.m_fY -= (frame->m_fHeight * fScale);
				}
				else
				{
					// just center it
					pos.m_fX = (pTargeting->m_fWidth / 2.0f); //- (frame->m_fWidth / 2.0f);
					pos.m_fY = (pTargeting->m_fHeight / 2.0f); //- (frame->m_fHeight / 2.0f);
				}
				
				UIComponentAddElement(pTargeting, texture, frame, pos, dwColor, NULL, FALSE, fScale, fScale);
			}
		}

		if (bShowBrackets && UITestFlag(UI_FLAG_SHOW_TARGET_BRACKETS) && bBracketsExist)
		{
			UI_RECT rect;
			UIGetUnitScreenBox(target, rect);
			//UIComponentAddBoxElement(component, rect.m_fX1, rect.m_fY1, rect.m_fX2, rect.m_fY2, NULL, GFXCOLOR_GREEN, 128);

			pTargeting->ppCorner[0]->m_dwColor = dwColor;
			pTargeting->ppCorner[1]->m_dwColor = dwColor2;
			pTargeting->ppCorner[2]->m_dwColor = dwColor;
			pTargeting->ppCorner[3]->m_dwColor = dwColor2;

			static const float scfLongRange = 50.0f;
			float fDistance = MIN(UnitsGetDistance(UIGetControlUnit(), target->vPosition), scfLongRange);

			float fScale = MAX(((scfLongRange - fDistance) / scfLongRange), 0.3f);
			for (int i=0; i<4; i++)
			{
				pTargeting->ppCorner[i]->m_fWidth = pTargeting->ppCorner[i]->m_pFrame->m_fWidth * fScale;
				pTargeting->ppCorner[i]->m_fHeight = pTargeting->ppCorner[i]->m_pFrame->m_fHeight * fScale;
				UIComponentHandleUIMessage(pTargeting->ppCorner[i], UIMSG_PAINT, 0, 0);
			}

			// ensure the box doesn't get too small
			if (rect.m_fX2 - rect.m_fX1 < pTargeting->ppCorner[0]->m_fWidth * 2.0f)
			{
				float fDiff = (pTargeting->ppCorner[0]->m_fWidth * 2.0f) - (rect.m_fX2 - rect.m_fX1);
				rect.m_fX2 += fDiff / 2.0f;
				rect.m_fX1 -= fDiff / 2.0f;
			}
			if (rect.m_fY2 - rect.m_fY1 < pTargeting->ppCorner[0]->m_fHeight * 2.0f)
			{
				float fDiff = (pTargeting->ppCorner[0]->m_fHeight * 2.0f) - (rect.m_fY2 - rect.m_fY1);
				rect.m_fY2 += fDiff / 2.0f;
				rect.m_fY1 -= fDiff / 2.0f;
			}

			pTargeting->ppCorner[0]->m_ActivePos.m_fX = rect.m_fX1;	
			pTargeting->ppCorner[0]->m_ActivePos.m_fY = rect.m_fY1;
			pTargeting->ppCorner[1]->m_ActivePos.m_fX = rect.m_fX2 - pTargeting->ppCorner[1]->m_fWidth;	
			pTargeting->ppCorner[1]->m_ActivePos.m_fY = rect.m_fY1;
			pTargeting->ppCorner[2]->m_ActivePos.m_fX = rect.m_fX1;	
			pTargeting->ppCorner[2]->m_ActivePos.m_fY = rect.m_fY2 - pTargeting->ppCorner[2]->m_fHeight;
			pTargeting->ppCorner[3]->m_ActivePos.m_fX = rect.m_fX2 - pTargeting->ppCorner[3]->m_fWidth;	
			pTargeting->ppCorner[3]->m_ActivePos.m_fY = rect.m_fY2 - pTargeting->ppCorner[3]->m_fHeight;

			for (int i=0; i<4; i++)
			{
				if (!UIComponentGetActive(pTargeting->ppCorner[i]))
					UIComponentHandleUIMessage(pTargeting->ppCorner[i], UIMSG_ACTIVATE, 0, 0);
				else if (pTargeting->ppCorner[i]->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
					pTargeting->ppCorner[i]->m_AnimEndPos = pTargeting->ppCorner[i]->m_ActivePos;
				else
					pTargeting->ppCorner[i]->m_Position = pTargeting->ppCorner[i]->m_ActivePos;
			}
		}
	}
	
	if (bBracketsExist &&
		(!target || !bShowBrackets || !UITestFlag(UI_FLAG_SHOW_TARGET_BRACKETS)))
	{
		for (int i=0; i<4; i++)
			UIComponentSetVisible(pTargeting->ppCorner[i], FALSE);

		pTargeting->ppCorner[0]->m_InactivePos.m_fX = pTargeting->ppCorner[0]->m_Position.m_fX = pTargeting->m_Position.m_fX;	
		pTargeting->ppCorner[0]->m_InactivePos.m_fY = pTargeting->ppCorner[0]->m_Position.m_fY = pTargeting->m_Position.m_fY;
		pTargeting->ppCorner[1]->m_InactivePos.m_fX = pTargeting->ppCorner[1]->m_Position.m_fX = pTargeting->m_Position.m_fX + pTargeting->m_fWidth;	
		pTargeting->ppCorner[1]->m_InactivePos.m_fY = pTargeting->ppCorner[1]->m_Position.m_fY = pTargeting->m_Position.m_fY;
		pTargeting->ppCorner[2]->m_InactivePos.m_fX = pTargeting->ppCorner[2]->m_Position.m_fX = pTargeting->m_Position.m_fX;	
		pTargeting->ppCorner[2]->m_InactivePos.m_fY = pTargeting->ppCorner[2]->m_Position.m_fY = pTargeting->m_Position.m_fY + pTargeting->m_fHeight;
		pTargeting->ppCorner[3]->m_InactivePos.m_fX = pTargeting->ppCorner[3]->m_Position.m_fX = pTargeting->m_Position.m_fX + pTargeting->m_fWidth;	
		pTargeting->ppCorner[3]->m_InactivePos.m_fY = pTargeting->ppCorner[3]->m_Position.m_fY = pTargeting->m_Position.m_fY + pTargeting->m_fHeight;

		for (int i=0; i<4; i++)
			if (pTargeting->ppCorner[i]->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)		
				CSchedulerCancelEvent(pTargeting->ppCorner[i]->m_tMainAnimInfo.m_dwAnimTicket);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TIME	timeToBeGone;
float	gethitx, gethity;

void UIGetHitIndicatorAnimate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}
	if (!game)
	{
		return;
	}
	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return;
	}

	TIMEDELTA timedelta = timeToBeGone - AppCommonGetCurTime();
	if (timedelta <= 0)
	{
		UIComponentSetVisible(component, FALSE);
		return;
	}    

	float pct = float(timedelta * 100 / component->m_dwAnimDuration) / 100.0f;
	DWORD dwAlpha = (DWORD)(pct * (float)0xff);

	// calc angle to rotate graphic
	VECTOR vPlrPos;
	float playerangle;
	GameGetControlPosition(AppGetCltGame(), &vPlrPos, &playerangle, NULL);
	float deltaX = vPlrPos.fX - gethitx;
	float deltaY = vPlrPos.fY - gethity;

	float deltaangle = atanf(deltaY / deltaX);

	float angle = deltaangle - playerangle;
	if (angle > TWOxPI)
	{
		angle -= TWOxPI;
	}
	if (angle < 0.0f)
	{
		angle += TWOxPI;
	}
	if (deltaX >= 0.0f)
	{
		angle += PI;
	}

	//WCHAR str[256];
	//PStrPrintf(str, arrsize(str), L"angle = %0.3f playerange= %0.3f diff = %0.3f ind angle = %0.3f\ndeltax = %0.3f deltay = %0.3f", deltaangle, playerangle, deltaangle - playerangle, angle, deltaX, deltaY);

	UIComponentRemoveAllElements(component);
	UIComponentAddRotatedElement(component, 
		texture, 
		component->m_pFrame, 
		UI_POSITION(), 
		UI_MAKE_COLOR(dwAlpha, component->m_dwColor),
		angle, 
		UI_POSITION(UIDefaultWidth() / 2.0f, UIDefaultHeight() / 2.0f));

	//UIComponentAddTextElement(component, NULL, UIComponentGetFont(component), UIComponentGetFontSize(component), str, UI_POSITION() );

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}
	component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIGetHitIndicatorAnimate, CEVENT_DATA((DWORD_PTR)component));

	return;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGetHitIndicatorDraw(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* unit = GameGetControlUnit(game);
	if (!unit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UIComponentSetVisible(component, TRUE);

	int sourcex = (int)wParam;
	int sourcey = (int)lParam;
	gethitx = (float)sourcex / 1000.0f;
	gethity = (float)sourcey / 1000.0f;
	timeToBeGone = AppCommonGetCurTime() + component->m_dwAnimDuration;

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}
	component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIGetHitIndicatorAnimate, CEVENT_DATA((DWORD_PTR)component));

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponConfigOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	//if (!UIComponentGetActive(component))
	//{
	//	return 0;
	//}

	//UI_INVSLOT* invslot = UICastToInvSlot(component);

	//UNIT* container = UIComponentGetFocusUnit(invslot);
	//if (!container)
	//{
	//	return 0;
	//}

	//int location = invslot->m_nInvLocation;
	//INVLOC_HEADER info;
	//if (!UnitInventoryGetLocInfo(container, location, &info))
	//{
	//	return 0;
	//}

	//if (!UIComponentCheckBounds(component))
	//{
	//	return 0;
	//}

	//UNIT* item = UnitInventoryGetByLocation(container, location);
	//if (!item)
	//{
	//	UISetHoverUnit(INVALID_ID, UIComponentGetId(invslot));
	//	return 1;
	//}
	//UISetHoverUnit(UnitGetId(item), UIComponentGetId(invslot));
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponConfigOnMouseHover(
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

	UI_WEAPON_CONFIG *pConfig = UICastToWeaponConfig(component);

	UNIT* container = UIComponentGetFocusUnit(pConfig);
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(container, pConfig->m_nHotkeyTag);
	if (!pTag)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UIComponentCheckBounds(pConfig->m_pWeaponPanel, x, y))
	{
		UNITID idItem = pTag->m_idItem[0];
		if (pTag->m_idItem[1] != INVALID_ID)		// if there's a secondary item
		{
			// see if the mouse is over it
			UI_POSITION posRelative(x, y);
			posRelative -= UIComponentGetAbsoluteLogPosition(pConfig->m_pWeaponPanel);
			UI_RECT rectLeft(0.0f, pConfig->m_pWeaponPanel->m_fHeight - pConfig->m_fDoubleWeaponFrameH, pConfig->m_fDoubleWeaponFrameW, pConfig->m_pWeaponPanel->m_fHeight);		// left-hand item
	
			if (UIInRect(posRelative, rectLeft))
			{
				idItem = pTag->m_idItem[1];
			}
		}

		UNIT *pItem = UnitGetById(AppGetCltGame(), idItem);
		if (!pItem)
		{
			idItem = INVALID_ID;
		}
		if (idItem != INVALID_ID)
		{
			UISetHoverTextItem(pConfig->m_pWeaponPanel, pItem);
			UISetHoverUnit(idItem, UIComponentGetId(pConfig->m_pWeaponPanel));
		}
		else
		{
			UISetHoverUnit(INVALID_ID, UIComponentGetId(pConfig->m_pWeaponPanel));
			if (msg == UIMSG_MOUSEHOVERLONG)
			{
				UIComponentMouseHoverLong(pConfig, msg, wParam, lParam);
			}
		}

	}
	else if (UIComponentCheckBounds(pConfig->m_pLeftSkillPanel, x, y))
	{
		UISetHoverTextSkill(pConfig->m_pLeftSkillPanel, UIGetControlUnit(), pTag->m_nSkillID[0]);
	}
	else if (UIComponentCheckBounds(pConfig->m_pRightSkillPanel, x, y))
	{
		UISetHoverTextSkill(pConfig->m_pRightSkillPanel, UIGetControlUnit(), pTag->m_nSkillID[1]);
	}
	else
	{
		UIClearHoverText();
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponConfigOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component)
	{
		UISetHoverUnit(INVALID_ID, INVALID_ID);
		UIClearHoverText();
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetWeaponConfigBoxClicked(
	UI_WEAPON_CONFIG *pConfig)
{
	float x, y;
	UIGetCursorPosition(&x, &y);

	UI_COMPONENT *pWeaponFrame = UIComponentFindChildByName(pConfig, "wc_weapon(s)");
	if (!pWeaponFrame ||
		!UIComponentCheckBounds(pWeaponFrame, x, y))
	{
		return -1;
	}

	if (pConfig->m_pWeaponPanel->m_pFrame == pConfig->m_pFrame2Weapon)
	{
		UI_POSITION posWeaponBox = UIComponentGetAbsoluteLogPosition(pConfig->m_pWeaponPanel);
		UI_RECT rect;
		rect.m_fX1 = posWeaponBox.m_fX;
		rect.m_fX2 = posWeaponBox.m_fX + pConfig->m_fDoubleWeaponFrameW;
		rect.m_fY2 = posWeaponBox.m_fY + pConfig->m_pWeaponPanel->m_fHeight;
		rect.m_fY1 = rect.m_fY2 - pConfig->m_fDoubleWeaponFrameH;
		if (UIInRect(UI_POSITION(x, y), rect))
		{
			return 1;
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponConfigOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_WEAPON_CONFIG *pConfig = UICastToWeaponConfig(component);
	GAME* game = AppGetCltGame();
	UNIT* container = UIComponentGetFocusUnit(pConfig);
	if (!game || !container || pConfig->m_nHotkeyTag == INVALID_LINK || pConfig->m_nInvLocation == INVALID_LINK)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nBoxClicked = sGetWeaponConfigBoxClicked(pConfig);
	if (nBoxClicked != 0 && nBoxClicked != 1) 
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int location = pConfig->m_nInvLocation;
	INVLOC_HEADER LocInfo;
	if (!UnitInventoryGetLocInfo(container, location, &LocInfo))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNITID idCursorUnit = UIGetCursorUnit();
	int nFromWeaponConfig = (g_UI.m_Cursor ? g_UI.m_Cursor->m_nFromWeaponConfig : -1);
//	BOOL bCurrentHotkey = (UnitGetStat(container, STATS_CURRENT_WEAPONCONFIG) == pConfig->m_nHotkeyTag - TAG_SELECTOR_WEAPCONFIG_HOTKEY1);

	UNIT* pCursorUnit = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!pCursorUnit)
	{
		UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(container, pConfig->m_nHotkeyTag);

		if (pTag &&
			pTag->m_idItem[nBoxClicked] != INVALID_ID)
		{
			UNIT *pItem = UnitGetById(game, pTag->m_idItem[nBoxClicked]);
			ASSERT_RETVAL(pItem, UIMSG_RET_NOT_HANDLED);

			UISetCursorUnit(UnitGetId(pItem), TRUE, pConfig->m_nHotkeyTag);
			UIPlayItemPickupSound(pItem);
		}
	}
	else
	{
		if (UnitGetUltimateOwner(pCursorUnit) != UIGetControlUnit())
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		if (ItemBelongsToAnyMerchant(pCursorUnit))
		{
			// we gots to buy it first
			if (!UIDoMerchantAction(pCursorUnit))
			{
				return UIMSG_RET_HANDLED_END_PROCESS;
			}
		}

		int nSuggestedPos = 0;
		if (nBoxClicked && !UnitIsA(pCursorUnit, UNITTYPE_TWOHAND))
		{
			nSuggestedPos = 1;
		}
		int nRealLocation = GetWeaponconfigCorrespondingInvLoc(nSuggestedPos);
		if (!IsAllowedUnitInv(pCursorUnit, nRealLocation, container))
		{
			// check the other pos
			nSuggestedPos = !nSuggestedPos;
			nRealLocation = GetWeaponconfigCorrespondingInvLoc(nSuggestedPos);
			if (!IsAllowedUnitInv(pCursorUnit, nRealLocation, container))
			{
				return UIMSG_RET_HANDLED_END_PROCESS;
			}
		}

		if (!InventoryItemCanBeEquipped(container, pCursorUnit))		
		{
			//return UIMSG_RET_HANDLED_END_PROCESS;
		}

		MSG_CCMD_ADD_WEAPONCONFIG msg;
		msg.idItem = UnitGetId(pCursorUnit);
		msg.bHotkey = (BYTE)pConfig->m_nHotkeyTag;
		msg.nFromWeaponConfig = nFromWeaponConfig;
		msg.bSuggestedPos = (BYTE)nSuggestedPos;
		c_SendMessage(CCMD_ADD_WEAPONCONFIG, &msg);
		
		UIPlayItemPutdownSound(pCursorUnit, container, location);

		// keep the item as a reference in the cursor
		float fXOffset = g_UI.m_Cursor->m_tItemAdjust.fXOffset;
		float fYOffset = g_UI.m_Cursor->m_tItemAdjust.fYOffset;
		UISetCursorUnit(msg.idItem, TRUE, -1 /*pConfig->m_nHotkeyTag*/, 0.0f, 0.0f, TRUE);
		g_UI.m_Cursor->m_tItemAdjust.fXOffset = fXOffset;
		g_UI.m_Cursor->m_tItemAdjust.fYOffset = fYOffset;

	}

	return UIMSG_RET_HANDLED_END_PROCESS;
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
UI_MSG_RETVAL UIWeaponConfigOnRButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_WEAPON_CONFIG *pConfig = UICastToWeaponConfig(component);
	GAME* game = AppGetCltGame();
	UNIT* container = UIComponentGetFocusUnit(pConfig);
	if (!game || !container || pConfig->m_nHotkeyTag == INVALID_LINK || pConfig->m_nInvLocation == INVALID_LINK)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nBoxClicked = sGetWeaponConfigBoxClicked(pConfig);
	if (nBoxClicked != 0 && nBoxClicked != 1) 
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int location = pConfig->m_nInvLocation;
	INVLOC_HEADER LocInfo;
	if (!UnitInventoryGetLocInfo(container, location, &LocInfo))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNITID idCursorUnit = UIGetCursorUnit();
//	BOOL bCurrentHotkey = (UnitGetStat(container, STATS_CURRENT_WEAPONCONFIG) == pConfig->m_nHotkeyTag - TAG_SELECTOR_WEAPCONFIG_HOTKEY1);

	UNIT* pCursorUnit = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!pCursorUnit)
	{
		UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(container, pConfig->m_nHotkeyTag);

		if (pTag &&
			pTag->m_idItem[nBoxClicked] != INVALID_ID)
		{
			UNIT *pItem = UnitGetById(game, pTag->m_idItem[nBoxClicked]);
			ASSERT_RETVAL(pItem, UIMSG_RET_NOT_HANDLED);

			if (UIIsMerchantScreenUp() &&	
				( (GetKeyState(VK_SHIFT) & 0x8000) &&		// the shift key is down
				!UnitInventoryIterate(pItem, NULL)))	// if the item does not contain any other items (mods)
			{
				// buy or sell the item
				if (UIDoMerchantAction(pItem))
				{
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
			else if (UIIsStashScreenUp() &&
				(GetKeyState(VK_SHIFT) & 0x8000))		// the shift key is down
			{
				// try to put the item in your stash
				if (!sInvLocTryPut( container, pItem, GlobalIndexGet( GI_INVENTORY_LOCATION_STASH) ))
				{
					if (PlayerIsSubscriber(container))
					{
						sInvLocTryPut( container, pItem, GlobalIndexGet( GI_INVENTORY_LOCATION_EXTRA_STASH) );
					}
				}

				return UIMSG_RET_HANDLED_END_PROCESS;
			}
			else if (AppIsHellgate())
			{
				c_InteractRequestChoices(pItem, component);

				//// We're not going to the server to get the possible choices right now because the client
				////   knows all it needs to about the items to display the radial menu.

				//INTERACT_MENU_CHOICE ptInteractChoices[MAX_INTERACT_CHOICES];
				//int nChoicesCount = 0;
				//ItemGetInteractChoices(UIGetControlUnit(), item, ptInteractChoices, nChoicesCount);
				//c_InteractDisplayChoices(UIGetControlUnit(), item, ptInteractChoices, nChoicesCount );

				return UIMSG_RET_HANDLED_END_PROCESS;
			}

		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponConfigOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_WEAPON_CONFIG *pConfig = UICastToWeaponConfig(component);
	GAME *game = AppGetCltGame();
	if (!game || AppGetState() != APP_STATE_IN_GAME)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* container = UIComponentGetFocusUnit(pConfig);
	if (!game || 
		!container || 
		UIGetControlUnit() != container ||
		pConfig->m_nHotkeyTag == INVALID_LINK || 
		pConfig->m_nInvLocation == INVALID_LINK)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(container, pConfig->m_nHotkeyTag);

	UIComponentRemoveAllElements(pConfig);
	UIComponentRemoveAllElements(pConfig->m_pWeaponPanel);
	UIComponentRemoveAllElements(pConfig->m_pLeftSkillPanel);
	UIComponentRemoveAllElements(pConfig->m_pRightSkillPanel);
	UIComponentRemoveAllElements(pConfig->m_pSelectedPanel);

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	ASSERT_RETVAL(texture, UIMSG_RET_NOT_HANDLED);

	pConfig->m_pWeaponPanel->m_pFrame = pConfig->m_pFrame1Weapon;
	UNIT *pWeapon1 = NULL;
	UNIT *pWeapon2 = NULL;
	if (pTag != NULL)
	{
		if (pTag->m_idItem[0] != INVALID_ID) 
			pWeapon1 = UnitGetById(game, pTag->m_idItem[0]);
		if (pTag->m_idItem[1] != INVALID_ID)
			pWeapon2 = UnitGetById(game, pTag->m_idItem[1]);

		if (pWeapon1 && !UnitIsA(pWeapon1, UNITTYPE_TWOHAND) ||
			pWeapon2 && !UnitIsA(pWeapon2, UNITTYPE_TWOHAND))
		{
			pConfig->m_pWeaponPanel->m_pFrame = pConfig->m_pFrame2Weapon;
		}
	}

	int nCurrentConfig = UnitGetStat(container, STATS_CURRENT_WEAPONCONFIG);
	BOOL bCurrentHotkey = nCurrentConfig == pConfig->m_nHotkeyTag - TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
	UNIT *pCurrentWeapon1 = NULL;
	UNIT *pCurrentWeapon2 = NULL;
	if (bCurrentHotkey)
	{
		pCurrentWeapon1 = pWeapon1;
		pCurrentWeapon2 = pWeapon2;
	}
	else
	{
		UNIT_TAG_HOTKEY *pCurTag = UnitGetHotkeyTag(container, nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
		if (pCurTag)
		{
			if (pCurTag->m_idItem[0] != INVALID_ID) 
				pCurrentWeapon1 = UnitGetById(game, pCurTag->m_idItem[0]);
			if (pCurTag->m_idItem[1] != INVALID_ID)
				pCurrentWeapon2 = UnitGetById(game, pCurTag->m_idItem[1]);
		} 
	}
	
	if (pConfig->m_pSelectedPanel &&
		bCurrentHotkey)		
	{
		if (pConfig->m_pWeaponPanel->m_pFrame == pConfig->m_pFrame2Weapon)
		{
			UIComponentAddElement(pConfig->m_pSelectedPanel, texture, pConfig->m_pDoubleSelectedFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_HOTPINK, NULL, TRUE);
		}
		else
		{
			UIComponentAddElement(pConfig->m_pSelectedPanel, texture, pConfig->m_pSelectedFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_HOTPINK, NULL, TRUE);
		}
	}

	UIComponentAddFrame(pConfig->m_pWeaponPanel);
	UIComponentAddFrame(pConfig->m_pLeftSkillPanel);
	UIComponentAddFrame(pConfig->m_pRightSkillPanel);

	UNIT* pCursorItem = UnitGetById(AppGetCltGame(), UIGetCursorUnit());
	if (pCursorItem &&
		UnitGetUltimateOwner(pCursorItem) == UIGetControlUnit())
	{

		// BSP - reenable this once we can drop skills here
		//if (UnitGetSkillID(pCursorItem) != INVALID_LINK)
		//{
		//	if (pConfig->m_pSkillLitFrame)
		//	{
		//		UIComponentAddElement(pConfig->m_pLeftSkillPanel, texture, pConfig->m_pSkillLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_WHITE, NULL, TRUE);
		//		UIComponentAddElement(pConfig->m_pRightSkillPanel, texture, pConfig->m_pSkillLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_WHITE, NULL, TRUE);
		//	}
		//}
		//else
		{
			int x, y;
			if (UnitInventoryFindSpace(container, pCursorItem, pConfig->m_nInvLocation, &x, &y))
			{
				if (UnitInventoryTest(container, pCursorItem, pConfig->m_nInvLocation, x, y))
				{
					BOOL bMeetsRequirements = FALSE;
					if (bCurrentHotkey && UnitIsA(pCursorItem, UNITTYPE_ONEHAND))
					{
						bMeetsRequirements = InventoryItemCanBeEquipped(container, pCursorItem, pCurrentWeapon1, NULL, INVLOC_NONE) || InventoryItemCanBeEquipped(container, pCursorItem, pCurrentWeapon2, NULL, INVLOC_NONE);
					}
					else
					{
						bMeetsRequirements = InventoryItemCanBeEquipped(container, pCursorItem, pCurrentWeapon1, pCurrentWeapon2, INVLOC_NONE);
					}
					if (pConfig->m_pWeaponLitFrame)
					{
						UIComponentAddElement(pConfig->m_pWeaponPanel, texture, pConfig->m_pWeaponLitFrame, UI_POSITION(0.0f, 0.0f), (bMeetsRequirements ? GFXCOLOR_WHITE : GFXCOLOR_RED), NULL, TRUE);
					}
				} 
			}
		}

		// !! We may want to allow modding items in the weaponconfig slots later

	//		else if (UnitIsA(pCursorUnit, UNITTYPE_MOD))
	//		{
	//			//Highlight this item if the mod can go in it
	//			int nLocation = INVLOC_NONE;
	//			BOOL bOpenSpace = ItemGetOpenLocation(item, pCursorUnit, FALSE, &nLocation, NULL, NULL);
	//			if (nLocation != INVLOC_NONE)
	//			{
	//				BOOL bMeetsRequirements = InventoryCheckRequirements(item, pCursorUnit, NULL, NULL, nLocation);
	//				if (bOpenSpace && bMeetsRequirements)
	//					UIComponentAddElement(component, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_GREEN, NULL, TRUE);
	//				else
	//					UIComponentAddElement(component, texture, invslot->m_pLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_RED, NULL, TRUE);
	//			}
	//		}
	}

	if (pTag == NULL)
	{
		return UIMSG_RET_HANDLED;
	}

	UNITID idHoverUnit = UIGetHoverUnit();
	// if this is the slot we've got the mouse over, highlight it
	if (!pCursorItem &&
		idHoverUnit != INVALID_ID &&
		(pTag->m_idItem[0] == idHoverUnit ||
		 pTag->m_idItem[1] == idHoverUnit))
	{
		UIComponentAddElement(pConfig->m_pWeaponPanel, texture, pConfig->m_pWeaponLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_WHITE, NULL, TRUE);
	}

	// if we're not going to be able to switch to this WC, show a red frame
	BOOL bCanSwitch = WeaponConfigCanSwapTo(game, container,  pConfig->m_nHotkeyTag - TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	if (!bCanSwitch)
	{
		UIComponentAddElement(pConfig->m_pWeaponPanel, texture, pConfig->m_pWeaponLitFrame, UI_POSITION(0.0f, 0.0f), GFXCOLOR_RED, NULL, TRUE);
	}

	if (pConfig->m_pWeaponPanel->m_pFrame == pConfig->m_pFrame2Weapon)
	{
		if (pWeapon1 /*&& pTag->m_idItem[0] != UIGetCursorUnit()*/)
		{
			if (pConfig->m_pWeaponBkgdFrameR && bCanSwitch)
			{
				int nColor = UIInvGetItemGridBackgroundColor(pWeapon1);
				if (nColor != INVALID_ID)
				{
					UIComponentAddElement(pConfig->m_pWeaponPanel, texture, pConfig->m_pWeaponBkgdFrameR, UI_POSITION(0.0f, 0.0f), GetFontColor(nColor), NULL, TRUE);
				}
			}
			UIComponentAddItemGFXElement(pConfig->m_pWeaponPanel, pWeapon1, 
				UI_RECT(pConfig->m_pWeaponPanel->m_fWidth - pConfig->m_fDoubleWeaponFrameW, 0.0f, pConfig->m_pWeaponPanel->m_fWidth, pConfig->m_fDoubleWeaponFrameH));
		}
		if (pWeapon2 /*&& pTag->m_idItem[1] != UIGetCursorUnit()*/)
		{
			if (pConfig->m_pWeaponBkgdFrameL && bCanSwitch)
			{
				int nColor = UIInvGetItemGridBackgroundColor(pWeapon2);
				if (nColor != INVALID_ID)
				{
					UIComponentAddElement(pConfig->m_pWeaponPanel, texture, pConfig->m_pWeaponBkgdFrameL, UI_POSITION(0.0f, 0.0f), GetFontColor(nColor), NULL, TRUE);
				}
			}
			UIComponentAddItemGFXElement(pConfig->m_pWeaponPanel, pWeapon2, 
				UI_RECT(0.0f, pConfig->m_pWeaponPanel->m_fHeight - pConfig->m_fDoubleWeaponFrameH, pConfig->m_fDoubleWeaponFrameW, pConfig->m_pWeaponPanel->m_fHeight));
		}
	}
	else
	{
		if (pWeapon1 /*&& pTag->m_idItem[0] != UIGetCursorUnit()*/)
		{
			if (pConfig->m_pWeaponBkgdFrame2 && bCanSwitch)
			{
				int nColor = UIInvGetItemGridBackgroundColor(pWeapon1);
				if (nColor != INVALID_ID)
				{
					UIComponentAddElement(pConfig->m_pWeaponPanel, texture, pConfig->m_pWeaponBkgdFrame2, UI_POSITION(0.0f, 0.0f), GetFontColor(nColor), NULL, TRUE);
				}
			}
			UIComponentAddItemGFXElement(pConfig->m_pWeaponPanel, pWeapon1, 
				UI_RECT(0.0f, 0.0f, pConfig->m_pWeaponPanel->m_fWidth, pConfig->m_pWeaponPanel->m_fHeight));
		}
	}


	// Now draw skills
	if (pTag->m_nSkillID[0] != INVALID_ID)
	{
		UI_RECT rect = UIComponentGetRect(pConfig->m_pLeftSkillPanel);
//		UI_TEXTURE_FRAME* pSkillIconBkgrd = UIGetStdSkillIconBackground(); 
		UI_POSITION pos(pConfig->m_pLeftSkillPanel->m_fWidth / 2.0f, pConfig->m_pLeftSkillPanel->m_fHeight / 2.0f);
		UIDrawSkillIcon(UI_DRAWSKILLICON_PARAMS(pConfig->m_pLeftSkillPanel, rect, pos, pTag->m_nSkillID[0], NULL, NULL, NULL, NULL, 255, FALSE, FALSE, FALSE, FALSE, FALSE, 1.0f, 1.0f, pConfig->m_pLeftSkillPanel->m_fWidth / UIGetAspectRatio(), pConfig->m_pLeftSkillPanel->m_fHeight));
	}
	if (pTag->m_nSkillID[1] != INVALID_ID)
	{
		UI_RECT rect = UIComponentGetRect(pConfig->m_pRightSkillPanel);
//		UI_TEXTURE_FRAME* pSkillIconBkgrd = UIGetStdSkillIconBackground(); 
		UI_POSITION pos(pConfig->m_pRightSkillPanel->m_fWidth / 2.0f, pConfig->m_pRightSkillPanel->m_fHeight / 2.0f);
		UIDrawSkillIcon(UI_DRAWSKILLICON_PARAMS(pConfig->m_pRightSkillPanel, rect, pos, pTag->m_nSkillID[1], NULL, NULL, NULL, NULL, 255, FALSE, FALSE, FALSE, FALSE, FALSE, 1.0f, 1.0f, pConfig->m_pLeftSkillPanel->m_fWidth / UIGetAspectRatio(), pConfig->m_pLeftSkillPanel->m_fHeight));
	}

	// add the label for the key
	if (pConfig->m_pKeyLabel)
	{
		const KEY_COMMAND &Key = InputGetKeyCommand(CMD_WEAPONCONFIG_1 + (pConfig->m_nHotkeyTag - TAG_SELECTOR_WEAPCONFIG_HOTKEY1));
		WCHAR szKeyName[256];
		if (InputGetKeyNameW(Key.KeyAssign[0].nKey, Key.KeyAssign[0].nModifierKey, szKeyName, arrsize(szKeyName)))
		{
			UILabelSetText(pConfig->m_pKeyLabel, szKeyName);
		}
	}


	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponConfigOnSetFocusStat(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (STAT_GET_STAT(lParam) == STATS_CURRENT_WEAPONCONFIG ||
		STAT_GET_STAT(lParam) == STATS_SKILL_LEFT ||
		STAT_GET_STAT(lParam) == STATS_SKILL_RIGHT )
	{
		UIWeaponConfigOnPaint(component, UIMSG_PAINT, 0, 0);
		return UIMSG_RET_HANDLED;
	}
	
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIUnitNameDisplayOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT *pPlayer = UIGetControlUnit();
	if (!pPlayer)
		return UIMSG_RET_NOT_HANDLED;

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay(component);

	ASSERT_RETVAL(pNameDisp, UIMSG_RET_NOT_HANDLED);

	if (pNameDisp->m_idUnitNameUnderCursor != INVALID_ID)
	{
		UNIT *pItem = UnitGetById(AppGetCltGame(), pNameDisp->m_idUnitNameUnderCursor);
//		ASSERT_RETVAL(pItem, UIMSG_RET_NOT_HANDLED);
		if (pItem)		// this may be too common for us to assert on.  Just fail quietly
		{
			float dist_sq = VectorDistanceSquared( pPlayer->vPosition, pItem->vPosition );

			if (dist_sq <= PICKUP_RADIUS_SQ)
			{
				if ( c_ItemTryPickup( AppGetCltGame(), pNameDisp->m_idUnitNameUnderCursor ) )
				{
					UIPlayItemPickupSound(pNameDisp->m_idUnitNameUnderCursor);
					pNameDisp->m_idUnitNameUnderCursor = INVALID_ID;
				}
			}
			else
			{
			}

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltOverviewMapButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate())
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(component);
		if ( pPlayer )
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_WORLDMAP );
	}

	UIToggleWorldMap();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltAutomapButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate())
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(component);
		if ( pPlayer )
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_AUTOMAP );
	}

	UICycleAutomap();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltQuestLogButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate())
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(component);
		if ( pPlayer )
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_QUESTLOG );
	}

	UIComponentActivate(UICOMP_QUEST_PANEL, TRUE);
		
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltBuddyListButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentActivate(UICOMP_BUDDYLIST, TRUE);
	UIComponentHandleUIMessage(component, UIMSG_MOUSELEAVE, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltGuildPanelButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentActivate(UICOMP_GUILDPANEL, TRUE);
	UIComponentHandleUIMessage(component, UIMSG_MOUSELEAVE, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltPartylistPanelButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsTugboat())
		UIComponentActivate(UICOMP_PARTYPANEL, TRUE);
	else
		UIComponentActivate(UICOMP_PLAYERMATCH_PANEL, TRUE);
	UIComponentHandleUIMessage(component, UIMSG_MOUSELEAVE, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltInventoryButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate())
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(component);
		if ( pPlayer )
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_INVENTORY );
	}

	//UIToggleShowItemNames(FALSE);
	//UIShowInventoryScreen();
	UIComponentActivate(UICOMP_INVENTORY, TRUE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltSkillsButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate())
	{
		UNIT *pPlayer = UIComponentGetFocusUnit(component);
		if ( pPlayer )
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_SKILLS );
	}

	//UIToggleShowItemNames(FALSE);
	UIComponentActivate(UICOMP_SKILLMAP, TRUE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltPerksButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	//UIToggleShowItemNames(FALSE);
	UIComponentActivate(UICOMP_PERKMAP, TRUE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltOptionsButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT * unit = GameGetControlUnit( AppGetCltGame() );
	if ( unit )
	{
		if ( UnitTestFlag( unit, UNITFLAG_QUESTSTORY ) )
		{
			c_QuestAbortCurrent( AppGetCltGame(), unit );
		}
	}

	UIToggleShowItemNames(FALSE);
	UIStopAllSkillRequests();

	c_PlayerClearAllMovement(AppGetCltGame());

	UIShowGameMenu();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltCustomerSupportButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	WCHAR uszURL[ REGION_MAX_URL_LENGTH ];
	RegionCurrentGetURL( AppGameGet(), RU_CUSTOMER_SUPPORT, uszURL, REGION_MAX_URL_LENGTH );
	//const WCHAR *szURL = GlobalStringGet(GS_URL_CUSTOMER_SUPPORT);
	if (uszURL[ 0 ])
	{
		ShowWindow(AppCommonGetHWnd(), SW_MINIMIZE);
		ShellExecuteW(NULL, NULL, uszURL, NULL, NULL, SW_SHOW );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIRadialMenuActivate(
	UNIT *pTarget,
	INTERACT_MENU_CHOICE *ptInteractChoices,
	int nMenuCount,
	UI_COMPONENT *pComponent /*=NULL*/,
	PGUID guid /*=INVALID_GUID*/)
{
	UI_COMPONENT *pRadialMenu = UIComponentGetByEnum(UICOMP_RADIAL_MENU);
	ASSERT_RETURN(pRadialMenu);

	if (!pTarget && guid == INVALID_GUID && !pComponent)
	{
		UIComponentSetVisible(pRadialMenu, FALSE);
		return;
	}

	if (pTarget && !UnitIsA(pTarget, UNITTYPE_ITEM) && !pComponent && guid == INVALID_GUID)
	{
		float fDistance = UnitsGetDistanceSquared( GameGetControlUnit( AppGetCltGame() ), pTarget );
		if ( fDistance > ( UNIT_INTERACT_DISTANCE_SQUARED ) )
			return;
	}

	//if (!UnitIsA(pTarget, UNITTYPE_OPENABLE) &&
	//	c_NPCGetInfoState( pTarget ) != INTERACT_INFO_NONE)
	//{
	//	return;
	//}

	pRadialMenu->m_dwData = pTarget ? UnitGetId(pTarget) : INVALID_ID;
	pRadialMenu->m_qwData = (QWORD)guid;

	float fAppWidth = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX();
	float fAppHeight = AppCommonGetWindowHeight() * UIGetScreenToLogRatioY();
	if (UICursorGetActive())
	{
		float fCursorX, fCursorY;
		UIGetCursorPosition(&fCursorX, &fCursorY);
		pRadialMenu->m_Position.m_fX = fCursorX - (pRadialMenu->m_fWidth / 2.0f);
		pRadialMenu->m_Position.m_fX = PIN(pRadialMenu->m_Position.m_fX, 0.0f, fAppWidth - pRadialMenu->m_fWidth);
		pRadialMenu->m_Position.m_fY = fCursorY - (pRadialMenu->m_fHeight / 2.0f);
		pRadialMenu->m_Position.m_fY = PIN(pRadialMenu->m_Position.m_fY, 0.0f, fAppHeight - pRadialMenu->m_fHeight);

		// set the cursor to the middle of the radial menu in case it got moved from the edge of the screen
		InputSetMousePosition(
			(pRadialMenu->m_Position.m_fX + (pRadialMenu->m_fWidth / 2.0f)) * UIGetLogToScreenRatioX(), 
			(pRadialMenu->m_Position.m_fY + (pRadialMenu->m_fHeight / 2.0f)) * UIGetLogToScreenRatioY());
	}
	else
	{
		pRadialMenu->m_Position.m_fX = (fAppWidth - pRadialMenu->m_fWidth) / 2.0f;
		pRadialMenu->m_Position.m_fY = (fAppHeight - pRadialMenu->m_fHeight) / 2.0f;
		InputSetMousePosition(AppCommonGetWindowWidth() / 2.0f, AppCommonGetWindowHeight() / 2.0f);
	}

	if( AppIsTugboat() )
	{
		pRadialMenu->m_InactivePos = pRadialMenu->m_Position;
	}
	UIClearHoverText();
	UIComponentSetActive(pRadialMenu, TRUE);
	UISetMouseOverrideComponent(pRadialMenu);

	int nCurrentButton = 0;
	BYTE byAlpha = UIComponentGetAlpha(pRadialMenu);
	int nButton = -1;
	float fHeight = 16;
	UI_COMPONENT *pChild = pRadialMenu->m_pFirstChild;
	while (pChild)
	{
		if( AppIsTugboat() )
		{
			if( UIComponentIsLabel( pChild ) )
			{
				fHeight = pChild->m_fHeight;
				// init and clear any previous user data
				pChild->m_dwData = (DWORD_PTR)INVALID_LINK;

				// do the next menu item if we have one
				BOOL bHasAction = FALSE;
				BOOL bDisabled = TRUE;
				int  nInteractMenu = INVALID_LINK;
				const INTERACT_MENU_DATA *pInteractMenuData = NULL;
				for (int iMenu = 0; iMenu < nMenuCount; iMenu++)
				{
					nInteractMenu = ptInteractChoices[ iMenu ].nInteraction;
					if (nInteractMenu != INVALID_LINK)
					{
						pInteractMenuData = InteractMenuGetData( nInteractMenu );
						if (!ptInteractChoices[ iMenu ].bDisabled &&
							iMenu > nButton )
						{
							nButton = iMenu;
							break;
						}
						else
						{
							pInteractMenuData = NULL;
						}
					}
				}

				if (pInteractMenuData)
				{
					bDisabled = FALSE;
					// assign text
					UILabelSetText(pChild, StringTableGetStringByIndex(pInteractMenuData->nStringTitle) );


					// assign action
					pChild->m_dwData = (DWORD)nInteractMenu;

					// this child has a menu
					bHasAction = TRUE;
					nCurrentButton++;
				}

				// set as active or inactive
				UIComponentSetActive( pChild, bHasAction && !bDisabled);
				UIComponentSetVisible( pChild, bHasAction && !bDisabled);

			}
		}
		else if (UIComponentIsButton(pChild))
		{
			UI_COMPONENT* pIconPanel = UIComponentFindChildByName(pChild, "icon panel");
			
			// init and clear any previous user data
			pChild->m_dwData = (DWORD_PTR)INVALID_LINK;
			
			// do the next menu item if we have one
			BOOL bHasAction = FALSE;
			BOOL bDisabled = FALSE;
			int  nInteractMenu = INVALID_LINK;
			int  nOverrideTooltipId = -1;
			const INTERACT_MENU_DATA *pInteractMenuData = NULL;
			for (int iMenu = 0; iMenu < nMenuCount; iMenu++)
			{
				nInteractMenu = ptInteractChoices[ iMenu ].nInteraction;
				if (nInteractMenu != INVALID_LINK)
				{
					pInteractMenuData = InteractMenuGetData( nInteractMenu );
					nOverrideTooltipId = ptInteractChoices[ iMenu ].nOverrideTooltipId;
					if (pInteractMenuData->nButtonNum == nCurrentButton)
					{
						bDisabled = ptInteractChoices[ iMenu ].bDisabled;
						break;
					}
					else
					{
						pInteractMenuData = NULL;
					}
				}
			}

			if (pInteractMenuData)
			{
				UIX_TEXTURE *pTexture = UIComponentGetTexture(pIconPanel);
				ASSERT_RETURN(pTexture);

				// assign image
				pIconPanel->m_pFrame = (UI_TEXTURE_FRAME *)StrDictionaryFind(pTexture->m_pFrames, pInteractMenuData->szIconFrame);

				if( !pChild->m_szTooltipText )
				{
					pChild->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
				}

				// assign tooltip
				if (  nOverrideTooltipId != -1)
					PStrCopy(pChild->m_szTooltipText, GlobalStringGet((GLOBAL_STRING)(nOverrideTooltipId)), UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
				else 
					PStrCopy(pChild->m_szTooltipText, StringTableGetStringByIndex(pInteractMenuData->nStringTooltip), UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);

				// assign action
				pChild->m_dwData = (DWORD)nInteractMenu;

				if (bDisabled)
				{
					pIconPanel->m_dwColor = GFXCOLOR_DKGRAY;
				}
				else
				{
					pIconPanel->m_dwColor = GFXCOLOR_WHITE;
				}

				// this child has a menu
				bHasAction = TRUE;
			}

			UIComponentSetAlpha(pIconPanel, byAlpha);
			UIComponentSetAlpha(pChild, byAlpha);
				
			// set as active or inactive
			UIComponentSetActive( pChild, bHasAction && !bDisabled);
			if (!bHasAction)
			{
				UIComponentSetAlpha(pChild, byAlpha / 2);
				pIconPanel->m_pFrame = NULL;
				if( pChild->m_szTooltipText )
				{
					pChild->m_szTooltipText[0] = L'\0';
				}
			}

			nCurrentButton++;
		}
		pChild = pChild->m_pNextSibling;
	}

	if( AppIsTugboat() )
	{
		pRadialMenu->m_fHeight = MAX( 24.0f, fHeight * (float)nCurrentButton );
	}
	UIComponentHandleUIMessage(pRadialMenu, UIMSG_PAINT, 0, 0);
	UISetCursorState(UICURSOR_STATE_POINTER);
	UIShowTargetIndicator(FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL RadialMenuBackgroundOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);
	if (UIComponentGetVisible(component))
	{
		DWORD dwRealTime = AppCommonGetRealTime();
	#define PERIOD_MILLISSECONDS	75000
		static const int ticks_per_period = (PERIOD_MILLISSECONDS * GAME_TICKS_PER_SECOND) / MSECS_PER_SEC;
		float fAngle = TWOxPI * (dwRealTime % ticks_per_period) / ticks_per_period;

		
		UI_POSITION pos = UIComponentGetAbsoluteLogPosition(component);
		pos.m_fX += component->m_fWidth / 2.0f;
		pos.m_fY += component->m_fHeight / 2.0f;
		//UI_POSITION pos = UI_POSITION((UI_DEFAULT_WIDTH-1.0f) / 2.0f, (UI_DEFAULT_HEIGHT-1.0f) / 2.0f); 
		//UIGetCursorPosition(&pos.m_fX, &pos.m_fY);

		UIComponentAddRotatedElement(component, UIComponentGetTexture(component), component->m_pFrame, UI_POSITION(), GFXCOLOR_HOTPINK, fAngle, pos);

		// schedule another
		if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
		}

		// schedule another one for one half second later
		component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + 10, component, UIMSG_PAINT, 0, 0);
	}
	else
	{
		component->m_bNeedsRepaintOnVisible = TRUE;
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRadialMenuOnButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentSetVisible(component, FALSE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRadialMenuOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// Eat it.  Just eat it.
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCloseRadialMenu(
	GAME* pGame,
	const CEVENT_DATA& data,
	DWORD )
{
	UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);

	// now do the action
	int nInteractMenu = (int)data.m_Data2;
	if (nInteractMenu != INVALID_LINK)
	{
		const INTERACT_MENU_DATA *pInteractMenuData = InteractMenuGetData( nInteractMenu );
		int nInteract = pInteractMenuData->nInteraction;
		ASSERTX_RETURN( nInteract != INVALID_LINK, "Expected interaction" );
		
		UNITID idTarget = (UNITID)data.m_Data1;
		UNIT *pTarget = UnitGetById( pGame, idTarget );
		const PGUID guid = ((PGUID)data.m_Data3 << 32) | ((PGUID)data.m_Data4);

//		if (pTarget || guid != INVALID_GUID)
		{
			c_PlayerInteract(AppGetCltGame(), pTarget, (UNIT_INTERACT)nInteract, FALSE, guid);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRadialMenuButtonOnButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if( AppIsHellgate() )
	{
		UI_BUTTONEX *pButton = UICastToButton(component);
		UIButtonBlinkHighlight(pButton);
		InputKeyIgnore(pButton->m_pBlinkData->m_nBlinkDuration + g_UI.m_dwStdAnimTime*2);
		InputMouseBtnIgnore(pButton->m_pBlinkData->m_nBlinkDuration + g_UI.m_dwStdAnimTime*2);
	}


	// get the target of the interaction stored in the parent control
	ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);

	// disable the radial menu so the player can't press another button
	UIComponentSetActiveByEnum(UICOMP_RADIAL_MENU, FALSE);

	// Do the action later, when the menu closes
	const PGUID guid = component->m_pParent->m_qwData;
	const DWORD_PTR qWord1= (DWORD_PTR)((guid >> 32) & 0xffffffff);
	const DWORD_PTR qWord2 =(DWORD_PTR)(guid & 0xffffffff);

	if( AppIsHellgate() )
	{
		CSchedulerRegisterEvent(AppCommonGetCurTime() + component->m_pBlinkData->m_nBlinkDuration, sCloseRadialMenu, CEVENT_DATA((DWORD_PTR)component->m_pParent->m_dwData, (DWORD_PTR)component->m_dwData, qWord1, qWord2));
	}
	else
	{
		CSchedulerRegisterEventImm(sCloseRadialMenu, CEVENT_DATA((DWORD_PTR)component->m_pParent->m_dwData, (DWORD_PTR)component->m_dwData, qWord1, qWord2));
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRadialMenuOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIRemoveMouseOverrideComponent(component);

	UIClearHoverText();
	UIShowTargetIndicator(TRUE);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWorldMapOnPostActiveChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (AppIsHellgate())		// not sure if this'll screw Tugboat up or not
	{
		if (component->m_pParent)
		{
			component->m_pParent->m_ScrollPos.m_fX = 0.0f;
			component->m_pParent->m_ScrollPos.m_fY = 0.0f;
		}
	}

    UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static float sfWMAnimPeriod = 16.0f;
static float sfAreaAnimPeriod = 64.0f;
void sWorldMapAnimate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_WORLD_MAP);
	if (!pComponent)
		return;

	UI_WORLD_MAP *pWorldMap = UICastToWorldMap(pComponent);

	if (!UIComponentGetVisible(pWorldMap))
		return;

	UI_POSITION pos;
	pos.m_fX = (float) data.m_Data1;
	pos.m_fY = (float) data.m_Data2;
	float fScale = (float)data.m_Data3;

	if( AppIsTugboat() )
	{

		if (!pWorldMap || !pWorldMap->m_pNewArea)
			return;

		UIComponentRemoveAllElements(pWorldMap->m_pIconsPanel);
		UIX_TEXTURE *pTexture = UIComponentGetTexture(pWorldMap->m_pIconsPanel);
		UIComponentAddElement(pWorldMap->m_pIconsPanel, pTexture, pWorldMap->m_pNewArea, pos, GFXCOLOR_WHITE, NULL, TRUE, fabs((fScale - sfAreaAnimPeriod)) / sfAreaAnimPeriod);
		fScale -= 1;
		if (fScale < 0)
			fScale = sfAreaAnimPeriod * 2;

		pWorldMap->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 10, sWorldMapAnimate, 
			CEVENT_DATA((DWORD_PTR)pos.m_fX, (DWORD_PTR)pos.m_fY, (DWORD_PTR)fScale) );

		return;
	}

	if (!pWorldMap || !pWorldMap->m_pYouAreHere)
		return;

	UIComponentRemoveAllElements(pWorldMap->m_pIconsPanel);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(pWorldMap->m_pIconsPanel);
	UI_RECT rectClip(-UIDefaultWidth() * 4.0f, -UIDefaultHeight() * 4.0f, UIDefaultWidth() * 4.0f, UIDefaultHeight() * 4.0f);	// could be big and we need to be able to scroll without repainting.
	UIComponentAddElement(pWorldMap->m_pIconsPanel, pTexture, pWorldMap->m_pYouAreHere, pos, 
		GFXCOLOR_HOTPINK, &rectClip, TRUE, fabs((fScale - sfWMAnimPeriod)) / (sfWMAnimPeriod * 1.5f));

	fScale -= 0.8f;
	if (fScale < 0.0f)
		fScale = sfWMAnimPeriod * 2.0f;

	pWorldMap->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 10, sWorldMapAnimate, 
		CEVENT_DATA((DWORD_PTR)pos.m_fX, (DWORD_PTR)pos.m_fY, (DWORD_PTR)fScale) );
}

static BOOL sWorldMapDrawLevel(
	UI_WORLD_MAP *pWorldMap,
	UIX_TEXTURE *pTexture,
	const LEVEL_DEFINITION *pLevelDef, 
	BOOL bVisited,
	int nLevelDef,
	int nCurLevelDef,
	UI_POSITION &pos,
	UI_RECT &rectExtents,
	UI_POSITION &posCurLocation, 
	UI_RECT * pClipRect)
{
	if (AppIsTugboat())
		return FALSE;

	const char *szFrame = (bVisited ? pLevelDef->szMapFrame : pLevelDef->szMapFrameUnexplored);
	if (!szFrame[0])
		return FALSE;

	UI_TEXTURE_FRAME *pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, szFrame);
	if (!pFrame)
		return FALSE;

	pos.m_fX = pLevelDef->nMapCol * pWorldMap->m_fColWidth;
	pos.m_fY = pLevelDef->nMapRow * pWorldMap->m_fRowHeight;

	pos.m_fX += (pWorldMap->m_fColWidth - pFrame->m_fWidth) / 2.0f;
	pos.m_fY += (pWorldMap->m_fRowHeight - pFrame->m_fHeight) / 2.0f;

	DWORD dwColor = GetFontColor(pLevelDef->nMapColor);
	UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture, pFrame, pos, dwColor, pClipRect);

	rectExtents.m_fX1 = MIN(rectExtents.m_fX1, pos.m_fX);
	rectExtents.m_fY1 = MIN(rectExtents.m_fY1, pos.m_fY);
	rectExtents.m_fX2 = MAX(rectExtents.m_fX2, pos.m_fX + pFrame->m_fWidth);
	rectExtents.m_fY2 = MAX(rectExtents.m_fY2, pos.m_fY + pFrame->m_fHeight);

	// draw the label
	WCHAR *puszLevelName = (WCHAR *)MALLOCZ(NULL, sizeof(WCHAR) * 256);
	ASSERT_RETFALSE(puszLevelName);

	PStrCopy(puszLevelName, StringTableGetStringByIndex(pLevelDef->nLevelDisplayNameStringIndex), 256);
	if (puszLevelName[0])
	{
#if ISVERSION(DEVELOPMENT)
		if (pLevelDef->eMapLabelPos == INVALID_LINK)
		{
			PStrCat(puszLevelName, L" - invalid WorldMapLabelPos", arrsize(puszLevelName));
		}
#endif
		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pWorldMap);
		UI_SIZE sizeLabel = DoWordWrapEx(puszLevelName, NULL, pWorldMap->m_fColWidth, pFont, pWorldMap->m_bNoScaleFont, UIComponentGetFontSize(pWorldMap) );

		int nLabelCol = pLevelDef->nMapCol;
		int nLabelRow = pLevelDef->nMapRow;
		if (e_UIAlignIsRight(pLevelDef->eMapLabelPos))
			nLabelCol += 1;
		if (e_UIAlignIsLeft(pLevelDef->eMapLabelPos))
			nLabelCol -= 1;
		if (e_UIAlignIsBottom(pLevelDef->eMapLabelPos))
			nLabelRow += 1;
		if (e_UIAlignIsTop(pLevelDef->eMapLabelPos))
			nLabelRow -= 1;
		UI_POSITION posLabel;
		posLabel.m_fX = (nLabelCol * pWorldMap->m_fColWidth) + pLevelDef->fMapLabelXOffs;
		posLabel.m_fY = (nLabelRow * pWorldMap->m_fRowHeight) + pLevelDef->fMapLabelYOffs;

		UI_SIZE size(pWorldMap->m_fColWidth, pWorldMap->m_fRowHeight);
		int eLabelAlign = (UIALIGN_NUM -1) - pLevelDef->eMapLabelPos;		// use the opposite alignment inside the box

		UIComponentAddTextElement(pWorldMap->m_pLevelsPanel, NULL, pFont, UIComponentGetFontSize(pWorldMap), 
			puszLevelName, posLabel, dwColor, pClipRect, eLabelAlign, &size, TRUE);

		if (g_UI.m_bDebugTestingToggle)
			UIComponentAddBoxElement(pWorldMap->m_pLevelsPanel, posLabel.m_fX, posLabel.m_fY, posLabel.m_fX + size.m_fWidth, posLabel.m_fY + size.m_fHeight, NULL, GFXCOLOR_GREEN, 128);

		float fAdjX1 = 0.0f;
		float fAdjX2 = 0.0f;
		float fAdjY1 = 0.0f;
		float fAdjY2 = 0.0f;
		if (sizeLabel.m_fWidth > pWorldMap->m_fColWidth)
		{
			if (e_UIAlignIsLeft(eLabelAlign))
			{
				fAdjX2 = sizeLabel.m_fWidth - pWorldMap->m_fColWidth;
			}
			if (e_UIAlignIsRight(eLabelAlign))
			{
				fAdjX1 = sizeLabel.m_fWidth - pWorldMap->m_fColWidth;
			}
			if (e_UIAlignIsCenterHoriz(eLabelAlign))
			{
				fAdjX1 = (sizeLabel.m_fWidth - pWorldMap->m_fColWidth)/2;
				fAdjX2 = (sizeLabel.m_fWidth - pWorldMap->m_fColWidth)/2;
			}
		}
		if (sizeLabel.m_fHeight > pWorldMap->m_fRowHeight)
		{
			if (e_UIAlignIsTop(eLabelAlign))
			{
				fAdjY2 = sizeLabel.m_fHeight - pWorldMap->m_fRowHeight;
			}
			if (e_UIAlignIsBottom(eLabelAlign))
			{
				fAdjY1 = sizeLabel.m_fHeight - pWorldMap->m_fRowHeight;
			}
			if (e_UIAlignIsCenterVert(eLabelAlign))
			{
				fAdjY1 = (sizeLabel.m_fHeight - pWorldMap->m_fRowHeight)/2;
				fAdjY2 = (sizeLabel.m_fHeight - pWorldMap->m_fRowHeight)/2;
			}
		}
		rectExtents.m_fX1 = MIN(rectExtents.m_fX1, posLabel.m_fX - fAdjX1);
		rectExtents.m_fY1 = MIN(rectExtents.m_fY1, posLabel.m_fY - fAdjY1);
		rectExtents.m_fX2 = MAX(rectExtents.m_fX2, posLabel.m_fX + size.m_fWidth + fAdjX2);
		rectExtents.m_fY2 = MAX(rectExtents.m_fY2, posLabel.m_fY + size.m_fHeight + fAdjY2);

	}

	if (puszLevelName)
	{
		FREE(NULL, puszLevelName);
	}

	BOOL bQuestIcon = FALSE;
	// see if we need to draw an icon for a quest
	if (pWorldMap->m_pNextStoryQuestIcon)
	{
		if (c_QuestIsThisLevelNextStoryLevel(AppGetCltGame(), nLevelDef))
		{
			bQuestIcon = TRUE;
			UI_POSITION posIcon;
			posIcon.m_fX = pLevelDef->nMapCol * pWorldMap->m_fColWidth;
			posIcon.m_fY = pLevelDef->nMapRow * pWorldMap->m_fRowHeight;

			posIcon.m_fX += (pWorldMap->m_fColWidth - pWorldMap->m_pNextStoryQuestIcon->m_fWidth) / 2.0f;
			posIcon.m_fY += (pWorldMap->m_fRowHeight - pWorldMap->m_pNextStoryQuestIcon->m_fHeight) / 2.0f;

			UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture, pWorldMap->m_pNextStoryQuestIcon, posIcon, GetFontColor(FONTCOLOR_STORY_QUEST), pClipRect);
		}
	}


	if (nLevelDef == nCurLevelDef)
	{
		posCurLocation.m_fX = pos.m_fX + (pFrame->m_fWidth / 2.0f);
		posCurLocation.m_fY = pos.m_fY + (pFrame->m_fHeight / 2.0f);
		pWorldMap->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(sWorldMapAnimate, 
			CEVENT_DATA(
				(DWORD_PTR)(posCurLocation.m_fX), 
				(DWORD_PTR)(posCurLocation.m_fY), 
				(DWORD_PTR)(sfWMAnimPeriod * 1.2f),
				(DWORD_PTR)(bQuestIcon)) );
	}

	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_TB_WorldMapRevealArea( int nRevealArea )
{
	if( nRevealArea == INVALID_ID )
	{
		return;
	}
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_WORLD_MAP);
	UI_WORLD_MAP *pWorldMap = UICastToWorldMap(component);
	
		
	UI_POSITION pos;

	MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( nRevealArea, INVALID_ID, pos.m_fX, pos.m_fY );

	//pos.m_fX *= pWorldMap->m_pLevelsPanel->m_fXmlWidthRatio;
	//pos.m_fY *= pWorldMap->m_pLevelsPanel->m_fXmlHeightRatio;

	if( pWorldMap->m_fOriginalMapWidth != 0 )
	{
		pos.m_fX *= pWorldMap->m_fWidth / pWorldMap->m_fOriginalMapWidth;
	}
	if( pWorldMap->m_fOriginalMapHeight != 0 )
	{
		pos.m_fY *= pWorldMap->m_fHeight / pWorldMap->m_fOriginalMapHeight;
	}


	pos.m_fX += pWorldMap->m_fIconOffsetX;
	pos.m_fY += pWorldMap->m_fIconOffsetY;
	//pos.m_fX /= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale);
	//pos.m_fY /= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale);

	UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( component->m_pParent, "Reveal" );

	pZoneCombo->m_Position = pos;
	pZoneCombo->m_ActivePos = pos;
	UIComponentSetActive( pZoneCombo, TRUE );
	UIComponentSetVisible( pZoneCombo, TRUE );
/*
	UIX_TEXTURE *pTexture = UIComponentGetTexture(pWorldMap->m_pIconsPanel);

	UIComponentAddElement(pWorldMap->m_pIconsPanel, pTexture, pWorldMap->m_pNewArea, pos, GFXCOLOR_WHITE, NULL, TRUE, 1, 1 );

	pWorldMap->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(sWorldMapAnimate, 
				CEVENT_DATA((DWORD_PTR)(pos.m_fX), 
					(DWORD_PTR)(pos.m_fY), 
					(DWORD_PTR)(sfWMAnimPeriod * 2)) );*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelConnectsToTown(
	GAME *pGame,
	int nLevelDefID,
	const LEVEL_DEFINITION *pLevelDef)
{
	if (pLevelDef->bTown)
		return TRUE;

    for (int i=0; i<MAX_LEVEL_MAP_CONNECTIONS; i++)
    {
	    if (pLevelDef->pnMapConnectIDs[i] > nLevelDefID)
	    {
			const LEVEL_DEFINITION *pOtherLevelDef = LevelDefinitionGet(pLevelDef->pnMapConnectIDs[i]);
			if (pOtherLevelDef->bTown ||
				sLevelConnectsToTown(pGame, pLevelDef->pnMapConnectIDs[i], pOtherLevelDef))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


#define UI_ZONE_COMBO		"zone combo"
#define UI_ZONE_LIST		"zone list"
#define MAP_OVERWORLD 10000
int	gnSelectedZoneMap = MAP_OVERWORLD; 

static
void sZoneComboSetSelection(UI_COMPONENT * pComboComp, int nZoneSelected )
{
	ASSERT_RETURN(pComboComp);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	UITextBoxClear(pCombo->m_pListBox);

	int nSelectionIndex = nZoneSelected;
	int nIndexAll = -1;
	int nIndices = 0;
	GAME * pGame = AppGetCltGame();
	int nZones = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_LEVEL_ZONES);
	WCHAR Str[MAX_XML_STRING_LENGTH];
	PStrPrintf(Str, arrsize(Str), L"World Map" );
	nIndices++;
	UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)MAP_OVERWORLD);
	for( int i = 0; i < nZones; i++ )
	{
		MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION* pLevelZone = (MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_ZONES, i );
		WCHAR Str[MAX_XML_STRING_LENGTH];

		PStrPrintf(Str, arrsize(Str), StringTableGetStringByIndex( pLevelZone->nZoneNameStringID ) );
		nIndices++;
		UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)i);

	}

	if( nSelectionIndex >= nIndices )
	{
		nSelectionIndex = 0;
	}
	nSelectionIndex = (nSelectionIndex < 0) ? nIndexAll : nSelectionIndex;

	UIComboBoxSetSelectedIndex(pCombo, nSelectionIndex);
}

static
void sZoneListSetSelection(UI_COMPONENT * pComboComp, int nZoneSelected )
{
	ASSERT_RETURN(pComboComp);
	UI_LISTBOX * pCombo = UICastToListBox(pComboComp);
	UITextBoxClear(pCombo);

	int nSelectionIndex = nZoneSelected;
	int nIndexAll = -1;
	int nIndices = 0;
	GAME * pGame = AppGetCltGame();
	int nZones = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_LEVEL_ZONES);
	WCHAR Str[MAX_XML_STRING_LENGTH];
	PStrPrintf(Str, arrsize(Str), L"World Map" );
	nIndices++;
	UIListBoxAddString(pCombo, Str, (QWORD)MAP_OVERWORLD);
	for( int i = 0; i < nZones; i++ )
	{
		MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION* pLevelZone = (MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_ZONES, i );
		WCHAR Str[MAX_XML_STRING_LENGTH];

		PStrPrintf(Str, arrsize(Str), StringTableGetStringByIndex( pLevelZone->nZoneNameStringID ) );
		nIndices++;
		UIListBoxAddString(pCombo, Str, (QWORD)i);

	}

	if( nSelectionIndex >= nIndices )
	{
		nSelectionIndex = 0;
	}
	nSelectionIndex = (nSelectionIndex < 0) ? nIndexAll : nSelectionIndex;
	UIListBoxSetSelectedIndex(pCombo, nSelectionIndex);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_TB_WorldMapSetZoneByArea( int nArea )
{
	UI_COMPONENT* worldmap_sheet = UIComponentGetByEnum(UICOMP_WORLDMAPSHEET);
	UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( worldmap_sheet, UI_ZONE_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pZoneCombo);
	int nZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nArea, NULL );
	sZoneComboSetSelection( pCombo, nZone + 1 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIZoneComboOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( component, UI_ZONE_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pZoneCombo);
	int nPlayerArea = LevelGetLevelAreaID( UnitGetLevel( UIGetControlUnit() ) );
	int nPlayerZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nPlayerArea, NULL );
	sZoneComboSetSelection( pCombo, nPlayerZone + 1 );


	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMapZoom( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( component->m_pParent, UI_ZONE_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pZoneCombo);
	sZoneComboSetSelection( pCombo, 0 );


	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWorldMapOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
		
	if ( AppIsHellgate() &&
		 component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_WORLD_MAP *pWorldMap = UICastToWorldMap(component);

	UIComponentRemoveAllElements(component);
	UIComponentAddFrame(component);

	if (!pWorldMap->m_pLevelsPanel || !pWorldMap->m_pConnectionsPanel || !pWorldMap->m_pIconsPanel)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentRemoveAllElements(pWorldMap->m_pLevelsPanel);
	UIComponentRemoveAllElements(pWorldMap->m_pConnectionsPanel);
	UIComponentRemoveAllElements(pWorldMap->m_pIconsPanel);

	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* pCtrlUnit = GameGetControlUnit(pGame);
	if (!pCtrlUnit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// expand our visited map stats into individual stats we can look at, we can
	// compress it once on the server and expand it only on the client because
	// its only the client who looks at it
	if (LevelCompressOrExpandMapStats( pCtrlUnit, LME_EXPAND ) == FALSE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	UI_POSITION pos;
	UI_POSITION posCurLocation(0.0f, 0.0f);
	UI_RECT rectExtents(FLT_MAX, FLT_MAX, 0.0f, 0.0f);

    if (!AppIsTugboat())
    {
	    ROOM* room = UnitGetRoom(pCtrlUnit);
	    LEVEL* curlevel = room ? RoomGetLevel(room) : NULL;
	    int nCurLevelDef = curlevel ? curlevel->m_nLevelDef : INVALID_LINK;
		UI_RECT rectClip(-UIDefaultWidth() * 4.0f, -UIDefaultHeight() * 4.0f, UIDefaultWidth() * 4.0f, UIDefaultHeight() * 4.0f);	// could be big and we need to be able to scroll without repainting.

	    DWORD dwLineColor = GetFontColor(FONTCOLOR_FSORANGE);
   
	    //UNIT_ITERATE_STATS_RANGE(pCtrlUnit, STATS_PLAYER_VISITED_LEVEL_BITFIELD, stats_list, ii, 64);
		int nDifficulty = UnitGetStat(pCtrlUnit, STATS_DIFFICULTY_CURRENT);
		int nRows = ExcelGetNumRows(NULL, DATATABLE_LEVEL);

		for(int nLevelDef = 0; nLevelDef < nRows; nLevelDef++)
		{
			const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet(nLevelDef);
			if (!pLevelDef)
				continue;

			int nValue = UnitGetStat(pCtrlUnit, STATS_PLAYER_VISITED_LEVEL_BITFIELD, pLevelDef->nBitIndex, nDifficulty);
		    if (nValue != WORLDMAP_HIDDEN)	// This level has been revealed
		    {

    
			    int nCurrentLevelStatus = nValue;
				nCurrentLevelStatus = (WORLD_MAP_STATE)MIN<int>((int)nCurrentLevelStatus, (int)WORLDMAP_VISITED);
			    if (sWorldMapDrawLevel(pWorldMap, pTexture, pLevelDef, nCurrentLevelStatus == WORLDMAP_VISITED, nLevelDef, nCurLevelDef, pos, rectExtents, posCurLocation, &rectClip))
			    {
				    // draw the connections
				    for (int i=0; i<MAX_LEVEL_MAP_CONNECTIONS; i++)
				    {
					    if (pLevelDef->pnMapConnectIDs[i] != INVALID_LINK)
					    {
							const LEVEL_DEFINITION* connnectedLevel = LevelDefinitionGet(pLevelDef->pnMapConnectIDs[i]);
							if(!connnectedLevel)
								continue;
						    int nConnectionStatus = UnitGetStat( pCtrlUnit, STATS_PLAYER_VISITED_LEVEL_BITFIELD, connnectedLevel->nBitIndex, nDifficulty);
							nConnectionStatus = (WORLD_MAP_STATE)MIN<int>((int)nConnectionStatus, (int)WORLDMAP_VISITED);
							//UnitGetStat(pCtrlUnit, STATS_PLAYER_VISITED_LEVEL, pLevelDef->pnMapConnectIDs[i]);
						    if (//pLevelDef->pnMapConnectIDs[i] > (int)nLevelDef && // only draw connections if a level has a higher id than the current
							    (nCurrentLevelStatus == WORLDMAP_VISITED ||		// if the level is connected to a level the player has visited
							    (/*nCurrentLevelStatus == WORLDMAP_REVEALED &&*/ nConnectionStatus == WORLDMAP_REVEALED)))	// or it's a connection between two revealed levels
						    {
							    UI_POSITION posLine = pos;
    
							    const LEVEL_DEFINITION *pOtherLevelDef = LevelDefinitionGet(pLevelDef->pnMapConnectIDs[i]);
							    ASSERT_CONTINUE(pOtherLevelDef);
    
								if (sLevelConnectsToTown(pGame, pLevelDef->pnMapConnectIDs[i], pOtherLevelDef) &&
									sLevelConnectsToTown(pGame, nLevelDef, pLevelDef))
								{
									dwLineColor = pWorldMap->m_dwStationConnectionColor;
								}
								else
								{
									dwLineColor = pWorldMap->m_dwConnectionColor;
								}

							    if (nConnectionStatus == WORLDMAP_HIDDEN)	// if this connection should be drawn only because it's next to a visited level
							    {
								    UI_POSITION posThrowaway;
								    sWorldMapDrawLevel(pWorldMap, pTexture, pOtherLevelDef, FALSE, pLevelDef->pnMapConnectIDs[i], nCurLevelDef, posThrowaway, rectExtents, posCurLocation, &rectClip);
							    }
    
							    UI_TEXTURE_FRAME *pLineFrame = NULL;
							    if (pOtherLevelDef->nMapCol  < pLevelDef->nMapCol && pOtherLevelDef->nMapRow  < pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pNWSEConnector;
							    if (pOtherLevelDef->nMapCol == pLevelDef->nMapCol && pOtherLevelDef->nMapRow  < pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pVertConnector;
							    if (pOtherLevelDef->nMapCol  > pLevelDef->nMapCol && pOtherLevelDef->nMapRow  < pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pNESWConnector;
							    if (pOtherLevelDef->nMapCol  > pLevelDef->nMapCol && pOtherLevelDef->nMapRow == pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pHorizConnector;
							    if (pOtherLevelDef->nMapCol  > pLevelDef->nMapCol && pOtherLevelDef->nMapRow  > pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pNWSEConnector;
							    if (pOtherLevelDef->nMapCol == pLevelDef->nMapCol && pOtherLevelDef->nMapRow  > pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pVertConnector;
							    if (pOtherLevelDef->nMapCol  < pLevelDef->nMapCol && pOtherLevelDef->nMapRow  > pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pNESWConnector;
							    if (pOtherLevelDef->nMapCol  < pLevelDef->nMapCol && pOtherLevelDef->nMapRow == pLevelDef->nMapRow)	
								    pLineFrame = pWorldMap->m_pHorizConnector;
								    
							    if (pLineFrame)
							    {
								    int nDeltaCol = pOtherLevelDef->nMapCol - pLevelDef->nMapCol;
								    int nDeltaRow = pOtherLevelDef->nMapRow - pLevelDef->nMapRow;
								    int nColDir = nDeltaCol ? nDeltaCol / abs(nDeltaCol) : 0;
								    int nRowDir = nDeltaRow ? nDeltaRow / abs(nDeltaRow) : 0;
								    posLine.m_fX += (pWorldMap->m_fColWidth / 2.0f) * nColDir;
								    posLine.m_fY += (pWorldMap->m_fRowHeight / 2.0f) * nRowDir;

									UI_RECT rectLineClip;
									rectLineClip.m_fX1 = (pLevelDef->nMapCol * pWorldMap->m_fColWidth) + (pWorldMap->m_fColWidth / 2.0f);
									rectLineClip.m_fY1 = (pLevelDef->nMapRow * pWorldMap->m_fRowHeight) + (pWorldMap->m_fRowHeight / 2.0f);
									rectLineClip.m_fX2 = (pOtherLevelDef->nMapCol * pWorldMap->m_fColWidth) + (pWorldMap->m_fColWidth / 2.0f);
									rectLineClip.m_fY2 = (pOtherLevelDef->nMapRow * pWorldMap->m_fRowHeight) + (pWorldMap->m_fRowHeight / 2.0f);
									rectLineClip.Normalize();
									if (nDeltaCol == 0)
									{
										rectLineClip.m_fX1 -= (pWorldMap->m_fColWidth / 2.0f);
										rectLineClip.m_fX2 += (pWorldMap->m_fColWidth / 2.0f);
									}
									if (nDeltaRow == 0)
									{
										rectLineClip.m_fY1 -= (pWorldMap->m_fRowHeight / 2.0f);
										rectLineClip.m_fY2 += (pWorldMap->m_fRowHeight / 2.0f);
									}

									if (!(nDeltaCol == 0 || nDeltaRow == 0 || abs(nDeltaRow) == abs(nDeltaCol)))
									{
										char msg[256];
										PStrPrintf(msg, arrsize(msg), "World Map: Level [%s] connection to level [%s] is not at 90 or 45 degree angle", pLevelDef->pszName, pOtherLevelDef->pszName);
										ASSERTX_CONTINUE(FALSE, msg);
									}

									if (!g_UI.m_bDebugTestingToggle)
									{
										while (UIInRect(UI_RECT(posLine.m_fX, posLine.m_fY, posLine.m_fX + pLineFrame->m_fWidth, posLine.m_fY + pLineFrame->m_fHeight), rectLineClip))
										{
											UIComponentAddElement(pWorldMap->m_pConnectionsPanel, pTexture, pLineFrame, posLine, dwLineColor, &rectLineClip);
											posLine.m_fX += pLineFrame->m_fWidth * nColDir;
											posLine.m_fY += pLineFrame->m_fHeight * nRowDir;
											if (nDeltaCol != 0 && nDeltaRow != 0) // diagonal
											{
												posLine.m_fX -= (pWorldMap->m_fDiagonalAdjustmentX * nColDir);
												posLine.m_fY -= (pWorldMap->m_fDiagonalAdjustmentY * nRowDir);
											}
										}
									}
									else
									{
										while (nDeltaCol != 0 || nDeltaRow != 0)
										{
											UIComponentAddElement(pWorldMap->m_pConnectionsPanel, pTexture, pLineFrame, posLine, dwLineColor, &rectClip);

											nDeltaCol -= nColDir;
											nDeltaRow -= nRowDir;

											// this won't reach more than a couple.  need to redo in order to stretch all the way.  Needs to repeat and clip
											posLine.m_fX += pLineFrame->m_fWidth * nColDir;
											posLine.m_fY += pLineFrame->m_fHeight * nRowDir;
										}
									}
							    }
						    }
					    }	
				    }
			    }
		    }
		}
	    //UNIT_ITERATE_STATS_END;
    
	    pWorldMap->m_fWidth = rectExtents.m_fX2;
	    pWorldMap->m_fHeight = rectExtents.m_fY2;
	    float fExtWidth = rectExtents.m_fX2 - rectExtents.m_fX1;
	    float fExtHeight = rectExtents.m_fY2 - rectExtents.m_fY1;
    
	    ASSERT_RETVAL(pWorldMap->m_pParent, UIMSG_RET_HANDLED);
    
	    if (posCurLocation.m_fX && posCurLocation.m_fY &&
		    fExtWidth > pWorldMap->m_pParent->m_fWidth)
	    {
		    // centered on the current location
		    pWorldMap->m_Position.m_fX = (pWorldMap->m_pParent->m_fWidth / 2.0f) - posCurLocation.m_fX;
    
		    // if we have extra room left, we can shift the map over
		    float fExtraX = pWorldMap->m_pParent->m_fWidth - (pWorldMap->m_Position.m_fX + fExtWidth);
		    if (fExtraX > 0)
		    {
			    pWorldMap->m_Position.m_fX += fExtraX;
		    }
		    if (pWorldMap->m_Position.m_fX > 0.0f)
			    pWorldMap->m_Position.m_fX = 0.0f;
	    }
	    else
	    {
		    // centered
		    pWorldMap->m_Position.m_fX = (pWorldMap->m_pParent->m_fWidth / 2.0f) - (fExtWidth / 2.0f);
	    }
    
	    if (posCurLocation.m_fX && posCurLocation.m_fY &&
		    fExtHeight > pWorldMap->m_pParent->m_fHeight)
	    {
		    // centered on the current location
		    pWorldMap->m_Position.m_fY = (pWorldMap->m_pParent->m_fHeight / 2.0f) - posCurLocation.m_fY;
    
		    // if we have extra room left, we can shift the map over
		    float fExtraY = pWorldMap->m_pParent->m_fHeight - (pWorldMap->m_Position.m_fY + fExtHeight);
		    if (fExtraY > 0)
		    {
			    pWorldMap->m_Position.m_fY += fExtraY;
		    }
		    if (pWorldMap->m_Position.m_fY > 0.0f)
			    pWorldMap->m_Position.m_fY = 0.0f;
	    }
	    else
	    {
		    // centered
		    pWorldMap->m_Position.m_fY = (pWorldMap->m_pParent->m_fHeight / 2.0f) - (fExtHeight / 2.0f);
	    }
    
		pWorldMap->m_pParent->m_ScrollMin.m_fX = MIN(pWorldMap->m_Position.m_fX, 0.0f);
		pWorldMap->m_pParent->m_ScrollMin.m_fY = MIN(pWorldMap->m_Position.m_fY, 0.0f);

		pWorldMap->m_pParent->m_ScrollMax.m_fX = MAX(pWorldMap->m_Position.m_fX + fExtWidth - pWorldMap->m_pParent->m_fWidth, 0.0f);
		pWorldMap->m_pParent->m_ScrollMax.m_fY = MAX(pWorldMap->m_Position.m_fY + fExtHeight - pWorldMap->m_pParent->m_fHeight, 0.0f);

		pWorldMap->m_Position.m_fX -= rectExtents.m_fX1;
		pWorldMap->m_Position.m_fY -= rectExtents.m_fY1;

		return UIMSG_RET_HANDLED;
    }
	else
    {
		UNIT* pPlayer = UIGetControlUnit();
	    int nPlayerArea = LevelGetLevelAreaID( UnitGetLevel( pPlayer ) );
		int nPlayerZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nPlayerArea, NULL );
		MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION *pLevelZone( NULL );	
		pLevelZone = (MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_ZONES, nPlayerZone );

		// if this is a travel map, we use the map dropdown zone override
		if ( component->m_dwParam != 1 )
		{
			nPlayerZone = gnSelectedZoneMap;
		}
		UI_COMPONENT* pPOIPool = UIComponentGetByEnum(UICOMP_WORLDMAP_POIS);
		UI_COMPONENT* pCurrentPOI = NULL;
		pCurrentPOI = pPOIPool->m_pFirstChild;

		// we're zoomed out in world map mode - we just want to see the overworld
		if( nPlayerZone == MAP_OVERWORLD )
		{
			MYTHOS_LEVELAREAS::CLevelAreasKnownIterator levelAreaIterator( UIGetControlUnit() );
			int mapLevelAreaID( INVALID_ID );

			// we need to add in the appropriate map backdrop for the zone we're in.
			UIX_TEXTURE* pMapTexture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, "worldmap_atlas");
			ASSERT_RETVAL(pMapTexture, UIMSG_RET_HANDLED);
			UI_TEXTURE_FRAME* pMapFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pMapTexture->m_pFrames, "worldmap");
			ASSERT_RETVAL(pMapFrame, UIMSG_RET_HANDLED);
			UI_POSITION pos( 0, 0 );
			UI_GFXELEMENT* pElement = UIComponentAddElement(pWorldMap->m_pLevelsPanel, pMapTexture, pMapFrame, pos, GFXCOLOR_WHITE, NULL, FALSE );
			ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);

			while ( (mapLevelAreaID = levelAreaIterator.GetNextLevelAreaID( MYTHOS_LEVELAREAS::KLVL_AREA_ITER_IGNORE_ZONE ) ) != INVALID_ID )
			{	
				const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pAreaDef = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( mapLevelAreaID );
				if( !pAreaDef->bShowsOnWorldMap )
				{
					continue;
				}
				float fScale = 1.5f;

	
				char uszMapIcon[16] = "\0";
				PStrPrintf( uszMapIcon, arrsize(uszMapIcon), "icon_%d", MYTHOS_LEVELAREAS::LevelAreaGetIconType( mapLevelAreaID ) );

				UI_TEXTURE_FRAME *pIcon = (UI_TEXTURE_FRAME*)StrDictionaryFind(pMapTexture->m_pFrames, uszMapIcon);
				if( !pIcon )
					pIcon = pWorldMap->m_pPartyIsHere;


				pos.m_fX = levelAreaIterator.GetLevelAreaWorldMapPixelX();
				pos.m_fY = levelAreaIterator.GetLevelAreaWorldMapPixelY();

				if( pWorldMap->m_fOriginalMapWidth != 0 )
				{
					pos.m_fX *= pWorldMap->m_fWidth / pWorldMap->m_fOriginalMapWidth;
				}
				if( pWorldMap->m_fOriginalMapHeight != 0 )
				{
					pos.m_fY *= pWorldMap->m_fHeight / pWorldMap->m_fOriginalMapHeight;
				}
				pos.m_fX += pWorldMap->m_fIconOffsetX;
				pos.m_fY += pWorldMap->m_fIconOffsetY;

				UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture, pIcon, pos, GFXCOLOR_WHITE, NULL, FALSE, pWorldMap->m_fIconScale * fScale, pWorldMap->m_fIconScale * fScale );

				if( mapLevelAreaID == nPlayerArea )
				{
					pos.m_fY += 16;
					UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture, pWorldMap->m_pYouAreHere, pos, GFXCOLOR_WHITE, NULL, FALSE, pWorldMap->m_fIconScale, pWorldMap->m_fIconScale );
				}
			}
		}
		else
		{


			VECTOR vDirection = UnitGetFaceDirection( pPlayer, TRUE );
			vDirection.fZ = 0;
			VectorNormalize( vDirection, vDirection );

			float fFacingAngle = (float)atan2f( -vDirection.fX, vDirection.fY);


			MYTHOS_LEVELAREAS::CLevelAreasKnownIterator levelAreaIterator( UIGetControlUnit() );
			int mapLevelAreaID( INVALID_ID );
			MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION* pLevelZone = (MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_ZONES, nPlayerZone );

			// we need to add in the appropriate map backdrop for the zone we're in.
			UIX_TEXTURE* pMapTexture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, pLevelZone->pszImageName);
			ASSERT_RETVAL(pMapTexture, UIMSG_RET_HANDLED);
			UI_TEXTURE_FRAME* pMapFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pMapTexture->m_pFrames, pLevelZone->pszImageFrame);
			ASSERT_RETVAL(pMapFrame, UIMSG_RET_HANDLED);
			UI_POSITION pos( 0, 0 );
			// we actually add this to the parent holder of the actual worldmap
			UI_GFXELEMENT* pElement = UIComponentAddElement(pWorldMap->m_pLevelsPanel, pMapTexture, pMapFrame, pos, GFXCOLOR_WHITE, NULL, FALSE );
			ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);

			UI_COMPONENT * pZoneList = UIComponentFindChildByName( component->m_pParent->m_pParent, UI_ZONE_LIST );
			UI_LISTBOX * pZoneCombo = pZoneList ? UICastToListBox(pZoneList) : NULL;
			if( pZoneCombo )
			{
				UITextBoxClear(pZoneCombo);
			}
			//int nIndex = 0;
			while ( (mapLevelAreaID = levelAreaIterator.GetNextLevelAreaID(MYTHOS_LEVELAREAS::KLVL_AREA_ITER_IGNORE_ZONE) ) != INVALID_ID )
			{			
				if( MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( mapLevelAreaID, NULL ) != nPlayerZone )
				{
					continue;
				}				
				if( MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( mapLevelAreaID ) )
				{
					if( mapLevelAreaID == nPlayerArea )
					{
						UNIT* pPlayer = UIGetControlUnit();
						VECTOR Position = UnitGetPosition( pPlayer );
						
						float fMapRatioX = pLevelZone->fZoneMapWidth / pLevelZone->fWorldWidth;
						float fMapRatioY = pLevelZone->fZoneMapHeight / pLevelZone->fWorldHeight;

						pos.m_fX = Position.fX * -fMapRatioX;
						pos.m_fY = Position.fY * -fMapRatioY;
						pos.m_fX += pLevelZone->fZoneOffsetX;
						pos.m_fY += pLevelZone->fZoneOffsetY;
						pos.m_fX *= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale);
						pos.m_fY *= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale);


						DWORD dwFlagsb = 0;
						SETBIT( dwFlagsb, ROTATION_FLAG_ABOUT_SELF_BIT );

						UIComponentAddRotatedElement(
							pWorldMap->m_pLevelsPanel, 
							pTexture,
							pWorldMap->m_pYouAreHere,
							pos,
							GFXCOLOR_WHITE,
							fFacingAngle,
							pos,
							NULL,
							TRUE,
							1.0f,
							dwFlagsb);
					}
					continue;
				}

				//float fScale = 1.5f;
/*

				char uszMapIcon[16] = "\0";
				PStrPrintf( uszMapIcon, arrsize(uszMapIcon), "icon_%d", MYTHOS_LEVELAREAS::LevelAreaGetIconType( mapLevelAreaID ) );

				UI_TEXTURE_FRAME *pIcon = (UI_TEXTURE_FRAME*)StrDictionaryFind(pMapTexture->m_pFrames, uszMapIcon);
				if( !pIcon )
					pIcon = pWorldMap->m_pPartyIsHere;

				pos.m_fX = levelAreaIterator.GetLevelAreaPixelX();
				pos.m_fY = levelAreaIterator.GetLevelAreaPixelY();
		
				if( pWorldMap->m_fOriginalMapWidth != 0 )
				{
					pos.m_fX *= pWorldMap->m_fWidth / pWorldMap->m_fOriginalMapWidth;
				}
				if( pWorldMap->m_fOriginalMapHeight != 0 )
				{
					pos.m_fY *= pWorldMap->m_fHeight / pWorldMap->m_fOriginalMapHeight;
				}
				pos.m_fX += pWorldMap->m_fIconOffsetX;
				pos.m_fY += pWorldMap->m_fIconOffsetY;

				//UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture, pIcon, pos, GFXCOLOR_WHITE, NULL, FALSE, pWorldMap->m_fIconScale * fScale, pWorldMap->m_fIconScale * fScale );

				if( pZoneCombo )
				{

					WCHAR Str[MAX_XML_STRING_LENGTH];

					MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( mapLevelAreaID, -1, &Str[0], MAX_XML_STRING_LENGTH, TRUE );

					UIListBoxAddString(pZoneCombo, Str, (QWORD)nIndex);
					nIndex++;
				}
				if( mapLevelAreaID == nPlayerArea )
				{
					pos.m_fY += 16;
					UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture, pWorldMap->m_pYouAreHere, pos, GFXCOLOR_WHITE, NULL, FALSE, pWorldMap->m_fIconScale, pWorldMap->m_fIconScale );
				}*/
			}

			//const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pAreaDef = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nPlayerArea );
			if( MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( nPlayerArea ) )
			{

				MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfInterest= PlayerGetPointsOfInterest( pPlayer );
				for( int t = 0; t < pPointsOfInterest->GetPointOfInterestCount(); t++ )
				{
					if( pPointsOfInterest->HasFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_Display ) )
					{
						const MYTHOS_POINTSOFINTEREST::PointOfInterest *pPointOfInterest = pPointsOfInterest->GetPointOfInterestByIndex( t );
						if( pPointOfInterest && pCurrentPOI )
						{

							if( !pPointsOfInterest->HasFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_IsQuestMarker ) )
							{
								VECTOR vPosition = VECTOR( (float)pPointOfInterest->nPosX, (float)pPointOfInterest->nPosY, 0 );
								float fDistance = VectorDistanceXY( vPosition, pPlayer->vPosition );
								if( fDistance > pPointOfInterest->pUnitData->flMaxAutomapRadius  )
								{
									continue;
								}
							}


							float fMapRatioX = pLevelZone->fZoneMapWidth / pLevelZone->fWorldWidth;
							float fMapRatioY = pLevelZone->fZoneMapHeight / pLevelZone->fWorldHeight;

							pos.m_fX = (float)pPointOfInterest->nPosX * -fMapRatioX;
							pos.m_fY = (float)pPointOfInterest->nPosY * -fMapRatioY;
							pos.m_fX += pLevelZone->fZoneOffsetX;
							pos.m_fY += pLevelZone->fZoneOffsetY;
							pos.m_fX -= pCurrentPOI->m_fWidth * .5f;
							pos.m_fY -= pCurrentPOI->m_fHeight * .5f;

							UIComponentActivate( pCurrentPOI, TRUE );
							pCurrentPOI->m_Position = pos;
							pCurrentPOI->m_ActivePos = pos;
							pCurrentPOI->m_InactivePos = pos;
							if( !pCurrentPOI->m_szTooltipText )
							{
								pCurrentPOI->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
							}
							const WCHAR *szTooltip = StringTableGetStringByIndex( pPointOfInterest->nStringId );
							PStrCopy(pCurrentPOI->m_szTooltipText, szTooltip, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
							UIComponentHandleUIMessage(pCurrentPOI, UIMSG_PAINT, 0, 0);
							pCurrentPOI = pCurrentPOI->m_pNextSibling;


						}
				
					}
				}
			}
			
		}
		// now hide any remaining bars that weren't used
		while( pCurrentPOI )
		{
			UIComponentSetVisible( pCurrentPOI, FALSE );
			pCurrentPOI = pCurrentPOI->m_pNextSibling;
		}		
   
	    int nMember = 0;

	    while (nMember < MAX_CHAT_PARTY_MEMBERS)
	    {
    
			    if (c_PlayerGetPartyMemberGUID(nMember) != INVALID_GUID)
			    {
				    int nArea = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
					BOOL bPVPWorld = c_PlayerGetPartyMemberPVPWorld(nMember, FALSE );
					int nPartyZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nArea, NULL );
				    if( nArea >= 0 &&
						nPartyZone == nPlayerZone &&
						bPVPWorld == PlayerIsInPVPWorld( UIGetControlUnit() ) )
				    {			
					
						
							UNIT* pPlayer = UIGetControlUnit();
							VECTOR Position = UnitGetPosition( pPlayer );

							float fMapRatioX = pLevelZone->fZoneMapWidth / pLevelZone->fWorldWidth;
							float fMapRatioY = pLevelZone->fZoneMapHeight / pLevelZone->fWorldHeight;

							pos.m_fX = Position.fX * -fMapRatioX;
							pos.m_fY = Position.fY * -fMapRatioY;
							pos.m_fX += pLevelZone->fZoneOffsetX;
							pos.m_fY += pLevelZone->fZoneOffsetY;

							UIComponentAddElement(pWorldMap->m_pLevelsPanel, pTexture,pWorldMap->m_pPartyIsHere, pos, GFXCOLOR_WHITE, NULL, FALSE, pWorldMap->m_fIconScale, pWorldMap->m_fIconScale );
					
				    }
    
			    }
    
			    nMember++;
    
	    }

	    return UIMSG_RET_HANDLED;
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIZoneComboOnSelect( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( component, UI_ZONE_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pZoneCombo);
	int nIndex = UIComboBoxGetSelectedIndex(pCombo);

	if (nIndex == INVALID_ID)
	{
		return UIMSG_RET_HANDLED;
	}

	QWORD nZoneIndexData = UIComboBoxGetSelectedData(pCombo);

	// if we change zones, hide any reveals
	if( (int)(nZoneIndexData) != gnSelectedZoneMap )
	{
		UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_WORLD_MAP);
		UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( component->m_pParent, "Reveal" );

		UIComponentSetActive( pZoneCombo, FALSE );
		UIComponentSetVisible( pZoneCombo, FALSE );
	}

	gnSelectedZoneMap = (int)(nZoneIndexData);

	UI_COMPONENT * pWorldMap = UIComponentFindChildByName( component->m_pParent, "World Map" );
	if( pWorldMap )
	{
		UIWorldMapOnPaint( pWorldMap, msg, wParam, lParam );
		// cancel any reveals
		CSchedulerCancelEvent(pWorldMap->m_tMainAnimInfo.m_dwAnimTicket);
	}
	return UIMSG_RET_HANDLED;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWorldMapOnPostActivate(
	UI_COMPONENT *component, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	// Hellgate's worldmap has to activate and de-activate its parent holder as well due to a naming issue (it's just easier this way)
	//if (component->m_pParent)
	//{
	//	if (nMessage == UIMSG_POSTACTIVATE)
	//	{
	//		UIComponentSetActive(component->m_pParent, TRUE);
	//	}
	//	if (nMessage == UIMSG_POSTINACTIVATE)
	//	{
	//		UIComponentSetActive(component->m_pParent, FALSE);
	//	}
	//}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;  
}	

inline void WarpToLevelArea( int nLevelAreaID,
							 int nLevelAreaDepth,
							 int nSkipRoad )
{
	MSG_CCMD_REQUEST_AREA_WARP message;
	message.nLevelArea = nLevelAreaID;
	message.nLevelDepth = nLevelAreaDepth > 0 ? nLevelAreaDepth - 1 : 0;
	message.bSkipRoad = nLevelAreaDepth == 0 ? nSkipRoad : FALSE;
	message.guidPartyMember =
		MYTHOS_LEVELAREAS::LevelAreaGetPartyMemberInArea(
		UIGetControlUnit(), nLevelAreaID, &message.idGameOfPartyMember);

	//Don't need to have a map if we're going to a party member.
	if(message.guidPartyMember != INVALID_GUID)
		message.nUnitID = MYTHOS_MAPS::GetUnitIDOfMapWithLevelAreaID( UIGetControlUnit(), nLevelAreaID );

	c_SendMessage( CCMD_REQUEST_AREA_WARP, &message );
	UISetCursorState( UICURSOR_STATE_POINTER );
	UIShowLoadingScreen( 0 );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITravelMenuWalk(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		// we open up the travel popup menu
		UI_COMPONENT *pMenu = UIComponentGetByEnum(UICOMP_TRAVEL_POPUP_MENU);
		if (!pMenu)
			return UIMSG_RET_NOT_HANDLED;  

		int mapLevelAreaID = ( pMenu->m_dwData != -1 )?(int)pMenu->m_dwData:(int)pMenu->m_dwParam;
		WarpToLevelArea( mapLevelAreaID, 0, 0 );
		UIComponentHandleUIMessage( pMenu, UIMSG_INACTIVATE, 0, 0);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITravelMenuWarp(
							   UI_COMPONENT* component,
							   int msg,
							   DWORD wParam,
							   DWORD lParam)
{

	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		// we open up the travel popup menu
		UI_COMPONENT *pMenu = UIComponentGetByEnum(UICOMP_TRAVEL_POPUP_MENU);
		if (!pMenu)
			return UIMSG_RET_NOT_HANDLED;  

		int mapLevelAreaID = (int)pMenu->m_dwParam;
		int mapLevelDepth = (int)pMenu->m_dwParam2;
		WarpToLevelArea( mapLevelAreaID, mapLevelDepth, 1 );
		UIComponentHandleUIMessage( pMenu, UIMSG_INACTIVATE, 0, 0);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	return UIMSG_RET_NOT_HANDLED;
}

BOOL UIFindConnectingLinker( int nLevelAreaAt,
							int nLevelAreaDestination, 
							int *nLevelAreasArrayTravelThrough,
							int &nLevelAreasCountTravelingThrough,
							BOOL bRequireVisitedHubs,
							int nIndex,
							int &nDepth )
{
	int nCount = ExcelGetCount( NULL, DATATABLE_LEVEL_AREAS_LINKER );

	//first we must find the first level area that links to area's together.
	nDepth++;
	if( nDepth > 10 )
	{
		nDepth--;
		return FALSE;
	}


	for( int t = 0; t < nCount; t++ )
	{
		if( t!= nIndex )
		{
			const MYTHOS_LEVELAREAS::LEVELAREA_LINK *linker = (const MYTHOS_LEVELAREAS::LEVELAREA_LINK *)ExcelGetData( NULL, DATATABLE_LEVEL_AREAS_LINKER, t );
			// find a link that has our destination in mind
			if( linker )
			{
				if( linker->m_LevelAreaIDLinkA == nLevelAreaDestination && ( !bRequireVisitedHubs || ( MYTHOS_LEVELAREAS::IsLevelAreaAHub(linker->m_LevelAreaIDLinkB) &&
					( MYTHOS_LEVELAREAS::CanWarpToSection( UIGetControlUnit(), linker->m_LevelAreaIDLinkB) != 0 ||
					  MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UIGetControlUnit(), linker->m_LevelAreaIDLinkB ) ) ) ) )
				{

					if( linker->m_LevelAreaIDLinkB == nLevelAreaAt )
					{
						nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = linker->m_LevelAreaIDLinkA;
						return TRUE;
					}
					if( UIFindConnectingLinker( nLevelAreaAt, linker->m_LevelAreaIDLinkB, nLevelAreasArrayTravelThrough, nLevelAreasCountTravelingThrough, bRequireVisitedHubs, t, nDepth ) )
					{
						nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = linker->m_LevelAreaIDLinkA;
						return TRUE;
					}
				}
				else if( linker->m_LevelAreaIDLinkB == nLevelAreaDestination  && ( !bRequireVisitedHubs || ( MYTHOS_LEVELAREAS::IsLevelAreaAHub(linker->m_LevelAreaIDLinkA) &&
					( MYTHOS_LEVELAREAS::CanWarpToSection( UIGetControlUnit(), linker->m_LevelAreaIDLinkA) != 0 ||
					MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UIGetControlUnit(), linker->m_LevelAreaIDLinkA ) ) ) ) )
				{
					if( linker->m_LevelAreaIDLinkA == nLevelAreaAt )
					{
						nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = linker->m_LevelAreaIDLinkB;
						return TRUE;
					}
					if( UIFindConnectingLinker( nLevelAreaAt, linker->m_LevelAreaIDLinkA, nLevelAreasArrayTravelThrough, nLevelAreasCountTravelingThrough, bRequireVisitedHubs, t, nDepth ) )
					{
						nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = linker->m_LevelAreaIDLinkB;
						return TRUE;
					}
				}

			}
		}		
	}
	nDepth--;
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//return FALSE if unable to reach destination
#define MAX_AREAS_TRAVEL_THROUGH 10
BOOL UIWorldMapPlotCourse( int nLevelAreaAt,
						  int nLevelAreaDestination, 
						  int *nLevelAreasArrayTravelThrough,
						  int &nLevelAreasCountTravelingThrough )
{
	ASSERT_RETFALSE( nLevelAreasArrayTravelThrough );	
	int hubAt = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nLevelAreaAt, NULL );
	int hubGoingTo = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nLevelAreaDestination, NULL );
	nLevelAreasCountTravelingThrough = 0;	
	nLevelAreasArrayTravelThrough[nLevelAreasCountTravelingThrough++]=nLevelAreaAt;	
	if( nLevelAreaAt == INVALID_ID ||
		nLevelAreaDestination == INVALID_ID ||
		hubAt == INVALID_ID ||
		hubGoingTo == INVALID_ID  )//||
		//MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UIGetControlUnit(), hubAt ) == FALSE ||
		//MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UIGetControlUnit(), hubGoingTo ) == FALSE )
	{
		return FALSE;
	}
	if( hubAt != nLevelAreaAt )
	{
		nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = hubAt;
	}
	//at this point we know the player has been to both hubs and is allowed to get to the final location	
	if( hubAt == hubGoingTo ) //we are at the correct location now
	{		
		if( hubGoingTo != nLevelAreaDestination ) //so now we going through an additional level area		
			nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = nLevelAreaDestination;		
		return TRUE;
	}
	//so now the hard part. How do we figure out the best case for the player to travel...
	//we have no idea how many hubs..



	//first we must find the first level area that links to area's together.
	int nDepth = 9;
	while( nDepth > 0 )
	{
		int nCurrentDepth = nDepth;
		if( !UIFindConnectingLinker( nLevelAreaAt, hubGoingTo, nLevelAreasArrayTravelThrough, nLevelAreasCountTravelingThrough, TRUE, -1, nCurrentDepth ) )
		{
			nCurrentDepth = nDepth;
			if( UIFindConnectingLinker( nLevelAreaAt, hubGoingTo, nLevelAreasArrayTravelThrough, nLevelAreasCountTravelingThrough, FALSE, -1, nCurrentDepth ) )
			{
				break;
			}
		}
		else
		{
			break;
		}
		nDepth--;
	}
	

	if( hubGoingTo != nLevelAreaDestination ) //so now we going through an additional level area		
		nLevelAreasArrayTravelThrough[ nLevelAreasCountTravelingThrough++ ] = nLevelAreaDestination;		
	return TRUE;
}

UI_MSG_RETVAL UIWorldMapOnLButtonUp( 
	UI_COMPONENT* component )
{
	UI_WORLD_MAP *pWorldMap = UICastToWorldMap(component);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	float OriginalX = x;
	float OriginalY = y;
	UI_POSITION pos;
	UIComponentCheckBounds(component, x, y, &pos);

	int nPlayerLevelArea =  LevelGetLevelAreaID( UnitGetLevel( UIGetControlUnit() )  );
	if( MYTHOS_LEVELAREAS::PlayerCanTravelToLevelArea( UIGetControlUnit(), nPlayerLevelArea ) == FALSE )
	{
		return UIMSG_RET_HANDLED;
	}

	UIClosePopupMenus();

	x -= pos.m_fX;
	y -= pos.m_fY;

	int nListBoxZone = INVALID_ID;
	UI_COMPONENT * pZoneList = UIComponentFindChildByName( component->m_pParent->m_pParent, UI_ZONE_LIST );
	UI_LISTBOX * pZoneCombo = pZoneList ? UICastToListBox(pZoneList) : NULL;
	if( pZoneCombo )
	{

		if (UIComponentCheckBounds(pZoneCombo, OriginalX, OriginalY))
		{
			nListBoxZone = pZoneCombo->m_nSelection;
		}

	} 

	MYTHOS_LEVELAREAS::CLevelAreasKnownIterator levelAreaIterator( UIGetControlUnit() );
	int mapLevelAreaID( INVALID_ID );
	BOOL isPlayerAtHub = MYTHOS_LEVELAREAS::IsLevelAreaAHub( nPlayerLevelArea );
	int nIndex = 0;
	while ( ( mapLevelAreaID = levelAreaIterator.GetNextLevelAreaID() ) != INVALID_ID)
	{		
		float iconX = levelAreaIterator.GetLevelAreaPixelX();
		float iconY = levelAreaIterator.GetLevelAreaPixelY();		
		//MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( mapLevelAreaID, INVALID_ID, iconX, iconY );
		//iconX *= pWorldMap->m_pLevelsPanel->m_fXmlWidthRatio;
		//iconY *= pWorldMap->m_pLevelsPanel->m_fXmlHeightRatio;
		if( pWorldMap->m_fOriginalMapWidth != 0 )
		{
			iconX *= pWorldMap->m_fWidth / pWorldMap->m_fOriginalMapWidth;
		}
		if( pWorldMap->m_fOriginalMapHeight != 0 )
		{
			iconY *= pWorldMap->m_fHeight / pWorldMap->m_fOriginalMapHeight;
		}
		iconX += pWorldMap->m_fIconOffsetX;
		iconY += pWorldMap->m_fIconOffsetY;
		iconX *= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale);
		iconY *= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale);

		// clicking the icon? this is soooo hacky.
		if( nIndex == nListBoxZone ||
			( x > iconX - MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE && x < iconX + MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE &&
			  y > iconY - MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE && y < iconY + MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE ) )
		{

			if( nPlayerLevelArea != mapLevelAreaID &&
				MYTHOS_LEVELAREAS::PlayerCanTravelToLevelArea( UIGetControlUnit(), mapLevelAreaID ) )
			{			

				int nLevelAreas[ MAX_AREAS_TRAVEL_THROUGH ];
				memset( nLevelAreas, -1, sizeof( nLevelAreas[0] ) * MAX_AREAS_TRAVEL_THROUGH );

				int pointsOfTravel( 0 );

				if( isPlayerAtHub &&
					UIWorldMapPlotCourse( nPlayerLevelArea, mapLevelAreaID, nLevelAreas, pointsOfTravel ) )
				{	
					int FurthestAreaID = nLevelAreas[1];
					int NearestAreaID = pointsOfTravel > 1 ? nLevelAreas[1] : -1;
					for( int m = 1; m < pointsOfTravel; m++ )
					{
						if( MYTHOS_LEVELAREAS::IsLevelAreaAHub(nLevelAreas[m]) &&
							( MYTHOS_LEVELAREAS::CanWarpToSection( UIGetControlUnit(), nLevelAreas[m]) != 0  ||
							MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UIGetControlUnit(), nLevelAreas[m]) ) )
						{
							FurthestAreaID = nLevelAreas[m];
						}

					}
					if( FurthestAreaID == mapLevelAreaID &&
						MYTHOS_LEVELAREAS::HasPlayerVisitedHub( UIGetControlUnit(), FurthestAreaID) )
					{
						// we open up the travel popup menu
						UI_COMPONENT *pMenu = UIComponentGetByEnum(UICOMP_TRAVEL_POPUP_MENU);
						if (!pMenu)
							return UIMSG_RET_NOT_HANDLED;  

						pMenu->m_Position.m_fX = g_UI.m_Cursor->m_Position.m_fX + 90;
						pMenu->m_Position.m_fY = g_UI.m_Cursor->m_Position.m_fY + g_UI.m_Cursor->m_fHeight - 54;


						pMenu->m_InactivePos = pMenu->m_Position;
						pMenu->m_ActivePos = pMenu->m_Position;

						UIComponentHandleUIMessage(pMenu, UIMSG_ACTIVATE, 0, 0);

						pMenu->m_dwParam = FurthestAreaID;
						pMenu->m_dwParam2 = MYTHOS_LEVELAREAS::CanWarpToSection( UIGetControlUnit(), FurthestAreaID);
						pMenu->m_dwData = NearestAreaID;
						return UIMSG_RET_HANDLED;
					}
					WarpToLevelArea( FurthestAreaID, 0, 1 );
					//UI_TB_HideWorldMapScreen( TRUE );
					return UIMSG_RET_HANDLED;
				}


				WarpToLevelArea( mapLevelAreaID, 0, 0 );
				//UI_TB_HideWorldMapScreen( TRUE );
				return UIMSG_RET_HANDLED;
			}
			else
			{
				UI_TB_HideWorldMapScreen( TRUE );
				return UIMSG_RET_HANDLED;
			}
		}	
		nIndex++;

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWorldMapOnLButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!AppIsTugboat())
		return UIMSG_RET_NOT_HANDLED;

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// if this isn't a travel map, don't interact.
	if ( component->m_dwParam != 1 )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	return UIWorldMapOnLButtonUp( component );
}



inline void FillInLevelAreaNamesBetweenHubs( WCHAR *uszDescription, int LevelAreaAt, int LevelAreaGoingTo, int &nMinDiff, int &nMaxDiff )
{
	int aSize = 1024;	
	
	int firstLevelArea = MYTHOS_LEVELAREAS::GetLevelAreaID_First_BetweenLevelAreas( LevelAreaAt, LevelAreaGoingTo );
	//this tells us what direction we are going...
	MYTHOS_LEVELAREAS::ELevelAreaLinker direction = (MYTHOS_LEVELAREAS::ShouldStartAtLinkA( LevelAreaAt, firstLevelArea ))?MYTHOS_LEVELAREAS::KLEVELAREA_LINK_NEXT:MYTHOS_LEVELAREAS::KLEVELAREA_LINK_PREVIOUS;
	
	//this tells us what is the first area we enter
	LevelAreaAt = firstLevelArea;

	//get depth of the area based off of the direction
	int nDepth = ( direction == MYTHOS_LEVELAREAS::KLEVELAREA_LINK_PREVIOUS )?(MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( LevelAreaAt ) - 1):0;
	

	WCHAR uszLastAreaName[ 1024];
	ZeroMemory( uszLastAreaName, sizeof( WCHAR) * 1024 );
	int MaxCount( 5);
	BOOL bFirstName( TRUE );

	int nTestMinDiff = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( LevelAreaAt );
	int nTestMaxDiff = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( LevelAreaAt );

	nMinDiff = ( nMinDiff > nTestMinDiff ) ? nMinDiff : nTestMinDiff;
	nMaxDiff = ( nMaxDiff > nTestMaxDiff ) ? nMaxDiff : nTestMaxDiff;
	while( MaxCount > 0 &&
		LevelAreaAt != LevelAreaGoingTo )
	{	
		MaxCount--;
		WCHAR uszLeveAreaName[ 1024 ];
		MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( LevelAreaAt, nDepth, &uszLeveAreaName[0], 1024 );				
		if( PStrCmp( uszLastAreaName, uszLastAreaName, 1024 ) == 0 )
		{
			if( !bFirstName )
			{
				PStrCat( uszDescription, L",\n", 1024 );
			}
			bFirstName = FALSE;
			UIColorCatString(uszDescription, aSize, FONTCOLOR_WHITE, uszLeveAreaName);
		}
		if( nDepth <= 0 && 
			direction == MYTHOS_LEVELAREAS::KLEVELAREA_LINK_PREVIOUS ) 
		{			
			LevelAreaAt = MYTHOS_LEVELAREAS::GetLevelAreaID_NextAndPrev_BetweenLevelAreas( LevelAreaAt, direction, nDepth, TRUE );
			nDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( LevelAreaAt ) - 1;
		}
		else if( direction == MYTHOS_LEVELAREAS::KLEVELAREA_LINK_PREVIOUS )
		{
			nDepth--;
		}
		else if( nDepth >= MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( LevelAreaAt ) -1 )
		{
			LevelAreaAt = MYTHOS_LEVELAREAS::GetLevelAreaID_NextAndPrev_BetweenLevelAreas( LevelAreaAt, direction, nDepth, TRUE );
			nDepth = 0;
		}
		else
		{
			nDepth++;
		}
		PStrCopy( uszLastAreaName, 1024, uszLeveAreaName, 1024 );
		if( LevelAreaAt != LevelAreaGoingTo )
		{
			nMinDiff = min( nMinDiff, MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( LevelAreaAt ) );
			nMaxDiff = max( nMaxDiff, MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( LevelAreaAt ) );
		}

	}
}

UI_MSG_RETVAL UIWorldMapOnMouseHover( UI_COMPONENT* component )
{
	//UI_WORLD_MAP *pWorldMap = UICastToWorldMap(component);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	UIComponentCheckBounds(component, x, y, &pos);
	float OriginalX = x;
	float OriginalY = y;
	x -= pos.m_fX;
	y -= pos.m_fY;
	//int nAreas = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_LEVEL_AREAS);
	LEVEL* pLevel = UnitGetLevel( UIGetControlUnit() );
	int nPlayerArea = LevelGetLevelAreaID( pLevel );
	int nPlayerZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nPlayerArea, NULL );
//	int nActualZone = nPlayerZone;
	//BOOL isPlayerAtHub = MYTHOS_LEVELAREAS::IsLevelAreaAHub( nPlayerArea );
	// if this is a travel map, we use the map dropdown zone override
	if ( component->m_dwParam != 1 )
	{
		nPlayerZone = gnSelectedZoneMap;
	}
	int nListBoxZone = INVALID_ID;
	UI_COMPONENT * pZoneList = UIComponentFindChildByName( component->m_pParent->m_pParent, UI_ZONE_LIST );
	UI_LISTBOX * pZoneCombo = pZoneList ? UICastToListBox(pZoneList) : NULL;
	if( pZoneCombo )
	{

		if (!UIComponentCheckBounds(pZoneCombo, OriginalX, OriginalY))
		{
			if( pZoneCombo->m_nHoverItem != -1 )
			{
				pZoneCombo->m_nHoverItem = -1;	
				UIComponentHandleUIMessage(pZoneCombo, UIMSG_PAINT, 0, 0);
			}
		}
		else
		{
			nListBoxZone = pZoneCombo->m_nHoverItem;
		}
		
	} 

	//UIComponentRemoveAllElements(pWorldMap->m_pConnectionsPanel);
	/*
	MYTHOS_LEVELAREAS::CLevelAreasKnownIterator levelAreaIterator( UIGetControlUnit() );
	int nLevelAreaID( INVALID_ID );
	int nIndex( 0 );
	float ScreenX;
	float ScreenY;
	{
		while ( (nLevelAreaID = levelAreaIterator.GetNextLevelAreaID( MYTHOS_LEVELAREAS::KLVL_AREA_ITER_IGNORE_ZONE ) ) != INVALID_ID )
		{			
			if( nPlayerZone != MAP_OVERWORLD && MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelAreaID, NULL ) != nPlayerZone )
			{
				continue;
			}
			float iconX = levelAreaIterator.GetLevelAreaPixelX();
			float iconY = levelAreaIterator.GetLevelAreaPixelY();
			if( nPlayerZone == MAP_OVERWORLD )
			{
				const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pAreaDef = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
				if( !pAreaDef->bShowsOnWorldMap )
				{
					continue;
				}
				iconX = levelAreaIterator.GetLevelAreaWorldMapPixelX();
				iconY = levelAreaIterator.GetLevelAreaWorldMapPixelY();
			}
			if( pWorldMap->m_fOriginalMapWidth != 0 )
			{
				iconX *= pWorldMap->m_fWidth / pWorldMap->m_fOriginalMapWidth;
			}
			if( pWorldMap->m_fOriginalMapHeight != 0 )
			{
				iconY *= pWorldMap->m_fHeight / pWorldMap->m_fOriginalMapHeight;
			}
			iconX += pWorldMap->m_fIconOffsetX;
			iconY += pWorldMap->m_fIconOffsetY;
			iconX *= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale);
			iconY *= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale);
			ScreenX = iconX;
			ScreenY = iconY;
			// clicking the icon? this is soooo hacky.
			if( nIndex == nListBoxZone ||
				( x > iconX - MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE && x < iconX + MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE &&
				y > iconY - MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE && y < iconY + MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE ) )
			{
				WCHAR uszDescription[ 1024 ] = L"\0";
				int nLevelAreas[ MAX_AREAS_TRAVEL_THROUGH ];
				memset( nLevelAreas, -1, sizeof( nLevelAreas[0] ) * MAX_AREAS_TRAVEL_THROUGH );

				int pointsOfTravel( 0 );
				// if we're not mousing over the area we're in,
				// let's see if the area we're over can be traveled to, and draw a line if so
				if( nPlayerZone != MAP_OVERWORLD &&
					nLevelAreaID != nPlayerArea &&
					MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelAreaID, NULL ) == nActualZone )
				{

					UIX_TEXTURE* pIconTexture = UIComponentGetTexture(pWorldMap->m_pIconsPanel);
					if (!pIconTexture)
					{
						return UIMSG_RET_NOT_HANDLED;
					}
					//UIComponentAddElement(pWorldMap->m_pConnectionsPanel, pIconTexture, pWorldMap->m_pYouAreHere, 	UI_POSITION( iconX / UIGetScreenToLogRatioX( pWorldMap->m_bNoScale), iconY  / UIGetScreenToLogRatioY( pWorldMap->m_bNoScale)), GFXCOLOR_WHITE, NULL, FALSE, .5f * pWorldMap->m_fIconScale, .5f * pWorldMap->m_fIconScale );

					
				}

				// is this a travel map?
				BOOL bTravelMap = component->m_dwParam == 1;


				//int PlayerLevel = UnitGetExperienceLevel( UIGetControlUnit() );
				int nTitleColor = FONTCOLOR_WHITE;			
				WCHAR insertString[ 1024 ];
				MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID, -1, &insertString[0], 1024, TRUE );
				UIColorCatString(uszDescription, arrsize(uszDescription), (FONTCOLOR)nTitleColor, &insertString[0] );
				WCHAR uszLevels[128] = L"\0";
				BOOL showDifficulty( FALSE );
				int nMinDiff = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID, UIGetControlUnit() );
				int nMaxDiff = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID, UIGetControlUnit() );
				if( bTravelMap && !MYTHOS_LEVELAREAS::LevelAreaIsHostile( nLevelAreaID )  )
				{
					//level area is a hub. Tell the player the difficulty of traveling there...
					if( nPlayerZone != MAP_OVERWORLD &&
						nLevelAreaID != nPlayerArea &&
						MYTHOS_LEVELAREAS::IsLevelAreaAHub( nPlayerArea ) &&
						MYTHOS_LEVELAREAS::IsLevelAreaAHub( nLevelAreaID ) )
					{
						PStrCat( uszDescription, L"\nVia\n", 1024 );

						for( int n = 0; n < pointsOfTravel - 1; n++ )
						{
							if( n == 0 )
							{
								nLevelAreaID = MYTHOS_LEVELAREAS::GetLevelAreaID_First_BetweenLevelAreas( nLevelAreas[n], nLevelAreas[n+1] );
								nMinDiff = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelAreaID, UIGetControlUnit() );
								nMaxDiff = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelAreaID, UIGetControlUnit() );
							}

							FillInLevelAreaNamesBetweenHubs( uszDescription, nLevelAreas[n], nLevelAreas[n+1], nMinDiff, nMaxDiff );
							//MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreas[n], 0, &insertString[0], 1024, TRUE );
							//PStrCat( uszDescription, &insertString[0], 1024 );
							if( n < pointsOfTravel - 2 )
							{
								MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreas[n + 1], -1, &insertString[0], 1024, TRUE );
								PStrCat( uszDescription, L"\n", 1024 );
								PStrCat( uszDescription, &insertString[0], 1024 );
								PStrCat( uszDescription, L",\n", 1024 );
							}




						}
						//
						showDifficulty = TRUE;
					}
				}
				else if( bTravelMap &&
					nLevelAreaID != nPlayerArea &&
					!MYTHOS_LEVELAREAS::IsLevelAreaAHub( nLevelAreaID ) )
				{
					showDifficulty = TRUE;
				}
				if( nPlayerArea == nLevelAreaID )
				{
					nTitleColor = FONTCOLOR_LIGHT_GRAY;
				}
				int nColor = MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( nLevelAreaID, UIGetControlUnit() );//  FONTCOLOR_GREEN;
				
				if( bTravelMap && showDifficulty )
				{
					if( nMinDiff == nMaxDiff )
					{
						PStrPrintf( uszLevels, arrsize(uszLevels), L"\n  Level : %d", nMaxDiff );
						UIColorCatString(uszDescription, arrsize(uszDescription), (FONTCOLOR)nColor, uszLevels);
					}
					else 
					{
						PStrPrintf( uszLevels, arrsize(uszLevels), L"\n  Level : %d-%d", nMinDiff, nMaxDiff );
						UIColorCatString(uszDescription, arrsize(uszDescription), (FONTCOLOR)nColor, uszLevels);
					}
				}
				if( nLevelAreaID == nPlayerArea )
				{
					PStrCat( uszDescription, L"\nCurrent Location", 512 );
				}
				if( !bTravelMap )
				{
				//	UIColorCatString(uszDescription, arrsize(uszDescription), FONTCOLOR_VERY_LIGHT_YELLOW, L"\n(You cannot travel from the World Map)");

				}
				else
				{
					if( MYTHOS_LEVELAREAS::PlayerCanTravelToLevelArea( UIGetControlUnit(), LevelGetLevelAreaID( pLevel ) ) == FALSE )
					{
						PStrCat( uszDescription, GlobalStringGet(GS_NO_OVERLAND_TRAVEL), 512 );
					}
				}
				//end really ugly messages
				for( int d = 0; d < MAX_ACTIVE_QUESTS_PER_UNIT; d++ )
				{
					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex(UIGetControlUnit(), d );

					if( pQuestTask &&
						QuestIsLevelAreaNeededByTask( UIGetControlUnit(), nLevelAreaID, pQuestTask ) )
					{
						int questTitleID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( UIGetControlUnit(), pQuestTask, KQUEST_STRING_TITLE );// QuestGetQuestTitle( pQuestTask->nParentQuest );
						PStrCat(uszDescription, L"\nQuest: ", arrsize(uszDescription));					
						UIColorCatString(uszDescription, 1024, (FONTCOLOR)nTitleColor, StringTableGetStringByIndex( questTitleID ));
						c_QuestFillFlavoredText( pQuestTask, INVALID_ID, uszDescription, 1024, 0 );						
					}
				}

				const UNIT *pMap = MYTHOS_MAPS::GetMapForAreaByAreaID( UIGetControlUnit(), nLevelAreaID );
				if( pMap &&
					!UnitIsA( pMap, UNITTYPE_MAP_KNOWN_PERMANENT ) )
				{
					PStrCat( uszDescription, L"\n Temporary Map ", arrsize(uszDescription));
				}

				if( MYTHOS_LEVELAREAS::LevelAreaGetMinPartySizeRecommended( nLevelAreaID ) > 1 )
				{
					UIColorCatString(uszDescription, arrsize(uszDescription), FONTCOLOR_LIGHT_RED, L"\nEpic Map");
				
				}

				UISetSimpleHoverText( ScreenX + 100, 
					ScreenY + 100, 
					ScreenX + g_UI.m_Cursor->m_fWidth + 100, 
					ScreenY + g_UI.m_Cursor->m_fHeight + 100, 
					uszDescription, 
					TRUE );		
				return UIMSG_RET_HANDLED;
			}
			nIndex++;

		}
	}


	int nMember = 0;
	int nActualMembers = 0;
	while (nMember < MAX_CHAT_PARTY_MEMBERS)
	{

		if (c_PlayerGetPartyMemberGUID(nMember) != INVALID_GUID)
		{
			int nArea = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
			BOOL bPVPWorld = c_PlayerGetPartyMemberPVPWorld(nMember, FALSE );
			if( nArea >= 0 &&
				bPVPWorld == PlayerIsInPVPWorld( UIGetControlUnit() ) )
			{					
				if( MYTHOS_LEVELAREAS::LevelAreaShowsOnMap( UIGetControlUnit(), nArea ) )
				{
					float iconX( 0.0f );
					float iconY( 0.0f );
					MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( nArea, INVALID_ID, iconX, iconY );						
					iconX *= pWorldMap->m_pLevelsPanel->m_fXmlWidthRatio;
					iconY *= pWorldMap->m_pLevelsPanel->m_fXmlHeightRatio;

					if( pWorldMap->m_fOriginalMapWidth != 0 )
					{
						iconX *= pWorldMap->m_fWidth / pWorldMap->m_fOriginalMapWidth;
					}
					if( pWorldMap->m_fOriginalMapHeight != 0 )
					{
						iconY *= pWorldMap->m_fHeight / pWorldMap->m_fOriginalMapHeight;
					}

					iconX += 16 * nActualMembers;						
					iconY += 64;
					iconX += pWorldMap->m_fIconOffsetX;
					iconY += pWorldMap->m_fIconOffsetY;
					iconX /= UIGetScreenToLogRatioX( pWorldMap->m_bNoScale);
					iconY /= UIGetScreenToLogRatioY( pWorldMap->m_bNoScale);
					if( x > iconX - 8 && x < iconX + 8 &&
						y > iconY - 8 && y < iconY + 8 )
					{
						UISetSimpleHoverText( g_UI.m_Cursor, c_PlayerGetPartyMemberName(nMember), TRUE );
						return UIMSG_RET_HANDLED;
					}
				}
			}


		}

		nMember++;

	}
	*/
	UIClearHoverText();
	return UIMSG_RET_HANDLED;
}
// !!!! This should all be localized
UI_MSG_RETVAL UIWorldMapOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!AppIsTugboat())
		return UIMSG_RET_NOT_HANDLED;

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	return UIWorldMapOnMouseHover( component );
	
}


UI_MSG_RETVAL UIWorldMapListBoxOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!AppIsTugboat())
		return UIMSG_RET_NOT_HANDLED;

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);

	if (!UIComponentGetVisible(component) || !UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_LISTBOX * pZoneCombo = UICastToListBox(component);
	if( pZoneCombo )
	{
		UIListBoxSetSelectedIndex(pZoneCombo, pZoneCombo->m_nHoverItem );
	} 

	UI_COMPONENT *pMap = UIComponentFindChildByName(component->m_pParent, "travel map");
	return UIWorldMapOnLButtonUp( pMap );
}

UI_MSG_RETVAL UIWorldMapListBoxOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!AppIsTugboat())
		return UIMSG_RET_NOT_HANDLED;

	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetVisible(component) || !UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pMap = UIComponentFindChildByName(component->m_pParent, "travel map");
	return UIWorldMapOnMouseHover( pMap );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIQuestTrackerClearAll(
	void)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_MONSTER_TRACKER);
	if (pComp)
	{
		UI_TRACKER *pTracker = UICastToTracker(pComp);
		for (int i=0; i < UI_TRACKER::MAX_TARGETS; i++)
		{
			pTracker->m_nTargetID[i] = INVALID_ID;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIQuestTrackerRemove(
	int nID)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_MONSTER_TRACKER);
	if (pComp)
	{
		UI_TRACKER *pTracker = UICastToTracker(pComp);
		for (int i=0; i < UI_TRACKER::MAX_TARGETS; i++)
		{
			if (pTracker->m_nTargetID[i] == nID)
			{
				pTracker->m_nTargetID[i] = INVALID_ID;
				return TRUE;
			}
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIQuestTrackerEnable(
	BOOL bTurnOn)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_MONSTER_TRACKER);
	if (pComp)
	{
		UI_TRACKER *pTracker = UICastToTracker(pComp);
		if (!bTurnOn)
		{
			pTracker->m_bTracking = FALSE;
			UIComponentActivate(pTracker, FALSE);
			pTracker->m_bUserActive = FALSE;
			UIComponentHandleUIMessage(pTracker, UIMSG_INACTIVATE, 0, 0);
		}
		else
		{	
			pTracker->m_bTracking = TRUE;
			UIComponentActivate(pTracker, TRUE);
			pTracker->m_bUserActive = TRUE;
			UIComponentHandleUIMessage(pTracker, UIMSG_PAINT, 0, 0);
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIQuestTrackerToggle(
	void)
{
	UI_COMPONENT *pTracker = UIComponentGetByEnum(UICOMP_MONSTER_TRACKER);
	if (pTracker)
	{
		UIQuestTrackerEnable(!UIComponentGetVisible(pTracker));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIQuestTrackerUpdatePos(
	int nID,
	const VECTOR & vector,
	int nType /*= 0*/,
	DWORD dwColor /*= GFXCOLOR_WHITE*/)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_MONSTER_TRACKER);
	if (pComp)
	{
		UI_TRACKER *pTracker = UICastToTracker(pComp);
		
		int nFirstEmpty = -1;
		for (int i=0; i < UI_TRACKER::MAX_TARGETS; i++)
		{
			if (pTracker->m_nTargetID[i] == nID)
			{
				pTracker->m_vTargetPos[i] = vector;
				return;
			}
			if (pTracker->m_nTargetID[i] == INVALID_ID &&
				nFirstEmpty == -1)
			{
				nFirstEmpty = i;
			}
		}
	
		if (nFirstEmpty >= 0)
		{
			pTracker->m_nTargetID[nFirstEmpty] = nID;
			pTracker->m_vTargetPos[nFirstEmpty] = vector;
			pTracker->m_nTargetFrameType[nFirstEmpty] = nType;
			pTracker->m_dwTargetColor[nFirstEmpty] = dwColor;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float RadialClipLine(
	float x1, 
	float y1, 
	float &x2, 
	float &y2, 
	float fMaxLen)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float fDist = sqrt(dx * dx + dy * dy);

	if (fDist < fMaxLen)
		return fDist;

	float fPercent = fMaxLen / fDist;

	x2 = (dx * fPercent) + x1;
	y2 = (dy * fPercent) + y1;

	return fMaxLen;
}

UI_MSG_RETVAL UITrackerOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_TRACKER *pTracker = UICastToTracker(component);
	ASSERT_RETVAL(pTracker, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(pTracker);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(pTracker);
	if (!pTexture)
		return UIMSG_RET_NOT_HANDLED;

	if (pTracker->m_pFrame)
		UIComponentAddFrame(pTracker);

	if (pTracker->m_bTracking)
	{
		VECTOR vEye(0.0f, 0.0f, 0.0f);
		VECTOR vUp(0.0f, 0.0f, -1.0f);
		VECTOR vFacing;

		const CAMERA_INFO* pCamera = c_CameraGetInfo(TRUE);
		if (!pCamera)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		VectorDirectionFromAngleRadians(vFacing, CameraInfoGetAngle(pCamera));
		vFacing.fZ = 0.0f;
		VectorNormalize(vFacing);

		float fRotate = PI / 2.0f;
		MATRIX mTransformationMatrix;
		MatrixIdentity(&mTransformationMatrix);
		MatrixRotationAxis(mTransformationMatrix, vUp, VectorToAngle(vEye, vFacing) + fRotate);

		UI_RECT cliprect(0.0f, 0.0f, pTracker->m_fWidth, pTracker->m_fHeight);
		float fXOffset = pTracker->m_fWidth / 2.0f;
		float fYOffset = pTracker->m_fHeight / 2.0f;
		UNIT* pPlayer = UIGetControlUnit();

		DWORD dwRotationFlags = 0;
		SETBIT(dwRotationFlags, ROTATION_FLAG_ABOUT_SELF_BIT);
		DWORD dwTime = (DWORD)(AppCommonGetCurTime() % (UINT64)pTracker->m_nSweepPeriodMSecs);
		float fSweeperAngle = 0.0f;
		if (pTracker->m_pSweeperFrame)
		{
			fSweeperAngle = NORMALIZE(TWOxPI * ((float)dwTime / (float)pTracker->m_nSweepPeriodMSecs), TWOxPI);
			UIComponentAddRotatedElement(pTracker, pTexture, pTracker->m_pSweeperFrame, UI_POSITION(fXOffset, fYOffset), GFXCOLOR_HOTPINK, fSweeperAngle, 
				UI_POSITION(fXOffset, fYOffset), NULL, FALSE, 1.0f, dwRotationFlags);
		}

		if (pTracker->m_pRingsFrame)
		{
			float fAngle = -TWOxPI * ((float)dwTime / (float)pTracker->m_nRingsPeriodMSecs);
			UIComponentAddRotatedElement(pTracker, pTexture, pTracker->m_pRingsFrame, UI_POSITION(fXOffset, fYOffset), GFXCOLOR_HOTPINK, fAngle, 
				UI_POSITION(fXOffset, fYOffset), NULL, FALSE, 1.0f, dwRotationFlags);
		}

		if (pTracker->m_pOverlayFrame)
		{
			UIComponentAddElement(pTracker, pTexture, pTracker->m_pOverlayFrame);
		}

		if (pPlayer)
		{
			VECTOR4 vec;
			VECTOR pt;

			for (int i=0; i < UI_TRACKER::MAX_TARGETS; i++)
			{
				float fFrameScale = 1.0f;
				int nFrameType = pTracker->m_nTargetFrameType[i];
				ASSERT_CONTINUE(nFrameType >= 0 && nFrameType < UI_TRACKER::MAX_TARGET_FRAMES);
				UI_TEXTURE_FRAME *pTargetFrame = pTracker->m_pTargetFrame[nFrameType];
				if (pTracker->m_nTargetID[i] != INVALID_ID &&
					pTargetFrame)
				{
					pt.fX = pTracker->m_vTargetPos[i].fX - pPlayer->vPosition.fX;
					pt.fY = pTracker->m_vTargetPos[i].fY - pPlayer->vPosition.fY;
					pt.fZ = pTracker->m_vTargetPos[i].fZ - pPlayer->vPosition.fZ;
					MatrixMultiply(&vec, &pt, &mTransformationMatrix);
					float x1 = fXOffset;
					float y1 = fYOffset;
					float x2 = vec.fX * pTracker->m_fScale + fXOffset;
					float y2 = vec.fY * pTracker->m_fScale + fYOffset;
					float fPercent = RadialClipLine(x1, y1, x2, y2, pTracker->m_fRadius) / pTracker->m_fRadius;

					BYTE byAlpha = (BYTE)(255.0f - (192.0f * fPercent));
					DWORD dwFrameColor = UI_MAKE_COLOR(byAlpha, pTracker->m_dwTargetColor[i]);
					UIComponentAddElement(pTracker, pTexture, pTargetFrame, UI_POSITION(x2, y2), dwFrameColor, NULL, TRUE, fFrameScale, fFrameScale );

					float fMarkerAngle = UIGetAngleRad(x1, y1, x2, y2);
					float fAngleDist = fSweeperAngle - fMarkerAngle;
					if (fAngleDist < 0.0f && fSweeperAngle < pTracker->m_fPingDistanceRads)
					{
						// correct for each of them being just on the other size of zero
						fAngleDist = (fSweeperAngle + TWOxPI) - fMarkerAngle;
					}
					fPercent = fAngleDist / pTracker->m_fPingDistanceRads;

					//{
					//	WCHAR szBuf[256];
					//	PStrPrintf(szBuf, arrsize(szBuf), L"sa = %0.2f, ma = %0.2f", fSweeperAngle, fMarkerAngle);
					//	UIComponentAddTextElement(pTracker, pTexture, UIComponentGetFont(pTracker), UIComponentGetFontSize(pTracker), szBuf, UI_POSITION());
					//}

					if (fAngleDist > 0.0f && fAngleDist < pTracker->m_fPingDistanceRads)
					{
						fAngleDist = fabs(fAngleDist);
						byAlpha = (BYTE)(255.0f - (255.0f * fPercent));
						fFrameScale = 1.0f + fPercent;
						dwFrameColor = UI_MAKE_COLOR(byAlpha, pTracker->m_dwTargetColor[i]);
						UIComponentAddElement(pTracker, pTexture, pTargetFrame, UI_POSITION(x2, y2), dwFrameColor, NULL, TRUE, fFrameScale, fFrameScale );
					}
				}
			}
		}

		if (pTracker->m_tUpdateAnimInfo.m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(pTracker->m_tUpdateAnimInfo.m_dwAnimTicket);
		}
		pTracker->m_tUpdateAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + GAME_TICKS_PER_SECOND / 2, pTracker, UIMSG_PAINT, 0, 0);
	}
	else
	{
		if (pTracker->m_pOverlayFrame)
		{
			UIComponentAddElement(pTracker, pTexture, pTracker->m_pOverlayFrame);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEnergyMeterResetFadeout(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UNIT *pFocus = UIComponentGetFocusUnit(component);

	if (!pFocus)
		return UIMSG_RET_NOT_HANDLED;

	DWORD dwDelayTime = (DWORD)component->m_dwParam2;

	component->m_fFading = 0.0f;
	if (component->m_bUserActive)
		UIComponentSetActive(component, TRUE);
	else
		UIComponentSetVisible(component, FALSE);

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	int nWeaponSkill = ItemGetPrimarySkill( pFocus ); 
	UNIT *pOwner = UnitGetOwnerUnit(pFocus);

	// If the weapon is still firing don't start a fadeout
	if (SkillGetIsOn(pOwner, nWeaponSkill, UnitGetId(pFocus)))
		return UIMSG_RET_NOT_HANDLED;

	if (component->m_bUserActive)
	{
		// register a future fade out
		if (UIComponentGetVisible(component))
		{
			// do it the normal way
			UIComponentActivate(component, FALSE, dwDelayTime, NULL, FALSE, FALSE, FALSE);	
		}
		else
		{
			// if the component's not visible yet, UIComponentActivate won't schedule a delayed inactivate, so do it manually
			if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
			{
				CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
			}
			//component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwDelayTime, UIDelayedFadeout, CEVENT_DATA((DWORD)component->m_idComponent, (DWORD)TRUE));
			component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwDelayTime, UIDelayedActivate, CEVENT_DATA((DWORD_PTR)component, UIMSG_INACTIVATE, FALSE, FALSE));
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEnergyMeterSetVisibility(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UNIT *pFocus = UIComponentGetFocusUnit(component);

	if (!pFocus)
		return UIMSG_RET_NOT_HANDLED;

	GAME * pGame = AppGetCltGame();
	int nWardrobe = UnitGetWardrobe( pFocus );
	if ( nWardrobe == INVALID_ID )
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_INVENTORYCHANGE)
	{
		// we only care if it's an inventory change to the weapons of our unit
		int nLoc = (int)lParam;
		if ((UNITID)wParam != UnitGetId(pFocus) ||
			(nLoc != INVALID_LINK &&
			!InvLocIsWeaponLocation(nLoc, pFocus)))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

	if (msg == UIMSG_WARDROBECHANGE)
	{
		// we only care if it's an inventory change to the weapons of our unit
		if ((UNITID)wParam != UnitGetId(pFocus))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		pWeapons[ i ] = WardrobeGetWeapon( pGame, nWardrobe, i );
	}

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while (pChild)
	{
		int nIdx = (int)pChild->m_dwParam - 1;
		if (nIdx >= 0 && nIdx < MAX_WEAPONS_PER_UNIT)
		{
			pChild->m_bUserActive = UnitUsesEnergy(pWeapons[nIdx]);
			UIComponentSetFocusUnit(pChild, UnitGetId(pWeapons[nIdx]));
			UIEnergyMeterResetFadeout(pChild, 0, 0, 0);		// sets it visible and starts the fadeout timer
		}
		pChild = pChild->m_pNextSibling;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadGunMod(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_GUNMOD* gunmod = UICastToGunMod(component);
	ASSERT_RETFALSE(gunmod);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(gunmod);
	ASSERT_RETFALSE(pTexture);

	char tex[256] = "";
	XML_LOADSTRING("icontexture", tex, "", 256);
	if (tex[0] != 0)
	{
		gunmod->m_pIconTexture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, tex);
	}

	char szSound[256] = "";
	XML_LOADSTRING("itemswitchsound", szSound, "", 256);
	if (szSound[0] != 0)
	{
		gunmod->m_nItemSwitchSound = ExcelGetLineByStringIndex(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_SOUNDS, szSound);
	}

	if (gunmod->m_pIconTexture)
	{
		xml.ResetChildPos();

		int nFramecount = 0;
		while (xml.FindChildElem("modtypeframe"))
		{
			gunmod->m_ppModLocFrame[nFramecount] = (UI_TEXTURE_FRAME*)StrDictionaryFind(gunmod->m_pIconTexture->m_pFrames, xml.GetChildData());
			gunmod->m_pnModLocFrameLoc[nFramecount] = ExcelGetLineByStringIndex(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_INVLOCIDX, xml.GetChildAttrib("invloc"));
			gunmod->m_pnModLocString[nFramecount] = StringTableGetStringIndexByKey(xml.GetChildAttrib("tooltip"));

			nFramecount++;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentFreeGunMod(
	UI_COMPONENT* component)
{
	UI_GUNMOD* gunmod = UICastToGunMod(component);
	ASSERT_RETURN(gunmod);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadModBox(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_MODBOX* modbox = UICastToModBox(component);
	ASSERT_RETFALSE(modbox);

	component->m_bVisible = FALSE;
	component->m_eState = UISTATE_INACTIVE;
	XML_LOADFLOAT("cellborder", modbox->m_fCellBorder, 0.0f);
	modbox->m_fCellBorder *= tUIXml.m_fXmlWidthRatio;

	if (!modbox->m_bReference)
		modbox->m_pLitFrame = NULL;
	UIX_TEXTURE* texture = UIComponentGetTexture(modbox);
	if (texture)
	{
		XML_LOADFRAME("litframe", texture, modbox->m_pLitFrame, "");
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadHUDTargeting(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_HUD_TARGETING *pTargeting = UICastToHUDTargeting(component);
	ASSERT_RETFALSE(pTargeting);

	pTargeting->eCursor = TCUR_DEFAULT;
	pTargeting->bShowBrackets = FALSE;

	pTargeting->ppCorner[0] = NULL;
	pTargeting->ppCorner[1] = NULL;
	pTargeting->ppCorner[2] = NULL;
	pTargeting->ppCorner[3] = NULL;

	pTargeting->pTargetingCenter = NULL;

	UI_COMPONENT *pChild = pTargeting->m_pFirstChild;
	while(pChild)
	{
		if (PStrCmp(pChild->m_szName, "selection TL")==0)
		{
			pTargeting->ppCorner[0] = pChild;
		}
		if (PStrCmp(pChild->m_szName, "selection TR")==0)
		{
			pTargeting->ppCorner[1] = pChild;
		}
		if (PStrCmp(pChild->m_szName, "selection BL")==0)
		{
			pTargeting->ppCorner[2] = pChild;
		}
		if (PStrCmp(pChild->m_szName, "selection BR")==0)
		{
			pTargeting->ppCorner[3] = pChild;
		}
		if (PStrCmp(pChild->m_szName, "targeting center")==0)
		{
			pTargeting->pTargetingCenter = pChild;
		}
		pChild = pChild->m_pNextSibling;
	}

	ASSERT_RETFALSE(pTargeting->pTargetingCenter);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE ( pTexture );
	for (int i = 0; i < NUM_TARGET_CURSORS; i++)
	{
		pTargeting->pCursorFrame[i] = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, StrDictGet(pTargetCursorEnumTbl,i));
	}

	memclear(pTargeting->m_pAccuracyFrame, sizeof(UI_TEXTURE_FRAME *) * UIALIGN_NUM);

	XML_LOADFRAME("accuracyframetop",		pTexture, pTargeting->m_pAccuracyFrame[UIALIGN_TOP], "");
	XML_LOADFRAME("accuracyframeright",		pTexture, pTargeting->m_pAccuracyFrame[UIALIGN_RIGHT], "");
	XML_LOADFRAME("accuracyframebottom",	pTexture, pTargeting->m_pAccuracyFrame[UIALIGN_BOTTOM], "");
	XML_LOADFRAME("accuracyframeleft",		pTexture, pTargeting->m_pAccuracyFrame[UIALIGN_LEFT], "");

	UIXMLLoadColor(xml, pTargeting, "accuracy1", pTargeting->m_dwAccuracyColor[0], GFXCOLOR_HOTPINK);
	UIXMLLoadColor(xml, pTargeting, "accuracy2", pTargeting->m_dwAccuracyColor[1], GFXCOLOR_HOTPINK);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadWeaponConfig(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_WEAPON_CONFIG* pConfig = UICastToWeaponConfig(component);
	ASSERT_RETFALSE(pConfig);

	XML_LOADINT("hotkeynum", pConfig->m_nHotkeyTag, INVALID_LINK);
	pConfig->m_nHotkeyTag += TAG_SELECTOR_WEAPCONFIG_HOTKEY1;
	XML_LOADEXCELILINK("invloc", pConfig->m_nInvLocation, DATATABLE_INVLOCIDX);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(pTexture);
	XML_LOADFRAME("frame1weap", pTexture, pConfig->m_pFrame1Weapon, "inventory 96x slot");
	XML_LOADFRAME("frame2weap", pTexture, pConfig->m_pFrame2Weapon, "double item slot");
	XML_LOADFRAME("weapon_lit_frame", pTexture, pConfig->m_pWeaponLitFrame, "");
	XML_LOADFRAME("skill_lit_frame", pTexture, pConfig->m_pSkillLitFrame, "");
	XML_LOADFRAME("selected_frame", pTexture, pConfig->m_pSelectedFrame, "");
	XML_LOADFRAME("double_selected_frame", pTexture, pConfig->m_pDoubleSelectedFrame, "");
	XML_LOADFLOAT("dblweaponframesize", pConfig->m_fDoubleWeaponFrameW, 0);

	XML_LOADFRAME("bkgd_frame_r", pTexture, pConfig->m_pWeaponBkgdFrameR, "");
	XML_LOADFRAME("bkgd_frame_l", pTexture, pConfig->m_pWeaponBkgdFrameL, "");
	XML_LOADFRAME("bkgd_frame_2", pTexture, pConfig->m_pWeaponBkgdFrame2, "");

	pConfig->m_fDoubleWeaponFrameH = pConfig->m_fDoubleWeaponFrameW;

	pConfig->m_fDoubleWeaponFrameW *= tUIXml.m_fXmlWidthRatio;
	pConfig->m_fDoubleWeaponFrameH *= tUIXml.m_fXmlHeightRatio;

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while (pChild)
	{
		if (PStrICmp(pChild->m_szName, "wc_leftskill")==0)
		{
			pConfig->m_pLeftSkillPanel = pChild;
		}
		if (PStrICmp(pChild->m_szName, "wc_rightskill")==0)
		{
			pConfig->m_pRightSkillPanel = pChild;
		}
		if (PStrICmp(pChild->m_szName, "selectedpanel")==0)
		{
			pConfig->m_pSelectedPanel = pChild;
		}
		if (PStrICmp(pChild->m_szName, "wc_weapon(s)")==0)
		{
			pConfig->m_pWeaponPanel = pChild;
			pConfig->m_pWeaponPanel->m_pFrame = pConfig->m_pFrame1Weapon;
			UIComponentRemoveAllElements(pConfig->m_pWeaponPanel);
			UIComponentAddFrame(pConfig->m_pWeaponPanel);
		}
		if (PStrICmp(pChild->m_szName, "wc_key_label")==0)
		{
			pConfig->m_pKeyLabel = pChild;
		}
		pChild = pChild->m_pNextSibling;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadWorldMap(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_WORLD_MAP *pWorldMap = UICastToWorldMap(component);
	ASSERT_RETFALSE(pWorldMap);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(pTexture);
	XML_LOADFRAME("vert_connect_frame", pTexture, pWorldMap->m_pVertConnector, "");
	XML_LOADFRAME("horiz_connect_frame", pTexture, pWorldMap->m_pHorizConnector, "");
	XML_LOADFRAME("nesw_connect_frame", pTexture, pWorldMap->m_pNESWConnector, "");
	XML_LOADFRAME("nwse_connect_frame", pTexture, pWorldMap->m_pNWSEConnector, "");
	XML_LOADFRAME("youarehere_frame", pTexture, pWorldMap->m_pYouAreHere, "");
	XML_LOADFRAME("pointofinterest_frame", pTexture, pWorldMap->m_pPointOfInterest, "");

	XML_LOADFLOAT("colwidth", pWorldMap->m_fColWidth, 64.0f);
	XML_LOADFLOAT("rowheight", pWorldMap->m_fRowHeight, 64.0f);
	pWorldMap->m_fColWidth *= tUIXml.m_fXmlWidthRatio;
	pWorldMap->m_fRowHeight *= tUIXml.m_fXmlHeightRatio;

	float fDiagonalAdjustment = 0.0f;
	XML_LOADFLOAT("diagonal_adj", fDiagonalAdjustment, 0.0f);
	XML_LOADFLOAT("diagonal_adj_x", pWorldMap->m_fDiagonalAdjustmentX, fDiagonalAdjustment);
	XML_LOADFLOAT("diagonal_adj_y", pWorldMap->m_fDiagonalAdjustmentY, fDiagonalAdjustment);
	pWorldMap->m_fDiagonalAdjustmentX *= tUIXml.m_fXmlWidthRatio;
	pWorldMap->m_fDiagonalAdjustmentY *= tUIXml.m_fXmlHeightRatio;

	pWorldMap->m_pLevelsPanel = UIComponentFindChildByName(component, "WM locations");
	pWorldMap->m_pConnectionsPanel = UIComponentFindChildByName(component, "WM connections");
	pWorldMap->m_pIconsPanel = UIComponentFindChildByName(component, "WM icons");

	UIXMLLoadColor(xml, pWorldMap, "connection_", pWorldMap->m_dwConnectionColor, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, pWorldMap, "station_connection_", pWorldMap->m_dwStationConnectionColor, GFXCOLOR_WHITE);

	XML_LOADFRAME("next_story_quest_frame", pTexture, pWorldMap->m_pNextStoryQuestIcon, "");

	// vvv Tugboat-specific
	XML_LOADFLOAT("original_width", pWorldMap->m_fOriginalMapWidth, 0);
	XML_LOADFLOAT("original_height", pWorldMap->m_fOriginalMapHeight, 0);
	XML_LOADFLOAT("iconoffsetx", pWorldMap->m_fIconOffsetX, 0);
	XML_LOADFLOAT("iconoffsety", pWorldMap->m_fIconOffsetY, 0);
	XML_LOADFLOAT("iconscale", pWorldMap->m_fIconScale, 1);

	XML_LOADFRAME("town_frame", pTexture, pWorldMap->m_pTownIcon, "");
	XML_LOADFRAME("dungeon_frame", pTexture, pWorldMap->m_pDungeonIcon, "");
	XML_LOADFRAME("cave_frame", pTexture, pWorldMap->m_pCaveIcon, "");
	XML_LOADFRAME("crypt_frame", pTexture, pWorldMap->m_pCryptIcon, "");
	XML_LOADFRAME("camp_frame", pTexture, pWorldMap->m_pCampIcon, "");
	XML_LOADFRAME("tower_frame", pTexture, pWorldMap->m_pTowerIcon, "");
	XML_LOADFRAME("church_frame", pTexture, pWorldMap->m_pChurchIcon, "");
	XML_LOADFRAME("citadel_frame", pTexture, pWorldMap->m_pCitadelIcon, "");
	XML_LOADFRAME("henge_frame", pTexture, pWorldMap->m_pHengeIcon, "");
	XML_LOADFRAME("farm_frame", pTexture, pWorldMap->m_pFarmIcon, "");
	XML_LOADFRAME("shrine_frame", pTexture, pWorldMap->m_pShrineIcon, "");
	XML_LOADFRAME("forest_frame", pTexture, pWorldMap->m_pForestIcon, "");
	XML_LOADFRAME("partyishere_frame", pTexture, pWorldMap->m_pPartyIsHere, "");
	XML_LOADFRAME("newarea_frame", pTexture, pWorldMap->m_pNewArea, "");
	XML_LOADFRAME("line_frame", pTexture, pWorldMap->m_pLineFrame, "");
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadTracker(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_TRACKER *pTracker = UICastToTracker(component);
	ASSERT_RETFALSE(pTracker);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(pTexture);

	for (int i=0; i < UI_TRACKER::MAX_TARGET_FRAMES; i++)
	{
		char szKey[256];
		PStrPrintf(szKey, arrsize(szKey), "target_frame%d", i);
		XML_LOADFRAME(szKey, pTexture, pTracker->m_pTargetFrame[i], "");
	}
	XML_LOADFRAME("sweeper_frame", pTexture, pTracker->m_pSweeperFrame, "");
	XML_LOADFRAME("crosshairs_frame", pTexture, pTracker->m_pCrosshairsFrame, "");
	XML_LOADFRAME("overlay_frame", pTexture, pTracker->m_pOverlayFrame, "");
	XML_LOADFRAME("rings_frame", pTexture, pTracker->m_pRingsFrame, "");

	XML_LOADFLOAT("scale", pTracker->m_fScale, 1.0f);
	XML_LOADFLOAT("radius", pTracker->m_fRadius, pTracker->m_fWidth);
	XML_LOADINT("sweep_period_msecs", pTracker->m_nSweepPeriodMSecs, 3 * MSECS_PER_SEC);
	XML_LOADINT("rings_period_msecs", pTracker->m_nRingsPeriodMSecs, pTracker->m_nSweepPeriodMSecs / 2);
	XML_LOADINT("ping_period_msecs", pTracker->m_nPingPeriodMSecs, MSECS_PER_SEC / 6);

	for (int i=0; i < UI_TRACKER::MAX_TARGETS; i++)
	{
		pTracker->m_vTargetPos[i].fX = 0.0f;
		pTracker->m_vTargetPos[i].fY = 0.0f;
		pTracker->m_vTargetPos[i].fZ = 0.0f;
		pTracker->m_nTargetID[i] = INVALID_ID;
	}

	float fSweeperSpeed = TWOxPI / pTracker->m_nSweepPeriodMSecs;		// rads / msec
	pTracker->m_fPingDistanceRads = (float)pTracker->m_nPingPeriodMSecs * fSweeperSpeed;

	pTracker->m_bTracking = FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesHellgate(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	
	//					   struct				type						name				load function				free function
	UI_REGISTER_COMPONENT( UI_GUNMOD,			UITYPE_GUNMOD,				"gunmod",			UIXmlLoadGunMod,			UIComponentFreeGunMod			);
	UI_REGISTER_COMPONENT( UI_MODBOX,			UITYPE_MODBOX,				"modbox",			UIXmlLoadModBox,			NULL							);
	UI_REGISTER_COMPONENT( UI_HUD_TARGETING,	UITYPE_HUD_TARGETING,		"hudtargeting",		UIXmlLoadHUDTargeting,		NULL							);
	UI_REGISTER_COMPONENT( UI_WEAPON_CONFIG,	UITYPE_WEAPON_CONFIG,		"weaponconfig",		UIXmlLoadWeaponConfig,		NULL							);
	UI_REGISTER_COMPONENT( UI_WORLD_MAP,		UITYPE_WORLDMAP,			"worldmap",			UIXmlLoadWorldMap,			NULL							);
	UI_REGISTER_COMPONENT( UI_TRACKER,			UITYPE_TRACKER,				"tracker",			UIXmlLoadTracker,			NULL							);

	UIMAP(UITYPE_GUNMOD,			UIMSG_INVENTORYCHANGE,		UIGunModScreenOnInventoryChange);
	UIMAP(UITYPE_GUNMOD,			UIMSG_LBUTTONDOWN,			UIGunModOnLButtonDown);
	UIMAP(UITYPE_MODBOX,			UIMSG_INVENTORYCHANGE,		UIModBoxOnInventoryChange);
	UIMAP(UITYPE_MODBOX,			UIMSG_PAINT,				UIModBoxOnPaint);
	UIMAP(UITYPE_MODBOX,			UIMSG_MOUSEHOVER,			UIModBoxOnMouseHover);
	UIMAP(UITYPE_MODBOX,			UIMSG_MOUSEHOVERLONG,		UIModBoxOnMouseHover);
	UIMAP(UITYPE_MODBOX,			UIMSG_MOUSELEAVE,			UIModBoxOnMouseLeave);
	UIMAP(UITYPE_MODBOX,			UIMSG_LBUTTONDOWN,			UIModBoxOnLButtonDown);
	UIMAP(UITYPE_UNIT_NAME_DISPLAY, UIMSG_LBUTTONCLICK,			UIUnitNameDisplayOnClick);
	UIMAP(UITYPE_HUD_TARGETING,		UIMSG_PAINT,				UITargetingOnPaint);
	UIMAP(UITYPE_HUD_TARGETING,		UIMSG_INVENTORYCHANGE,		UITargetingOnPaint);

	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_PAINT,				UIWeaponConfigOnPaint);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_INVENTORYCHANGE,		UIWeaponConfigOnPaint);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_MOUSEOVER,			UIWeaponConfigOnMouseOver);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_MOUSEHOVER,			UIWeaponConfigOnMouseHover);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_MOUSEHOVERLONG,		UIWeaponConfigOnMouseHover);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_MOUSELEAVE,			UIWeaponConfigOnMouseLeave);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_LBUTTONDOWN,			UIWeaponConfigOnLButtonDown);
	UIMAP(UITYPE_WEAPON_CONFIG,		UIMSG_RBUTTONDOWN,			UIWeaponConfigOnRButtonDown);

	UIMAP(UITYPE_WORLDMAP,		UIMSG_PAINT,					UIWorldMapOnPaint);						
	UIMAP(UITYPE_WORLDMAP,		UIMSG_POSTACTIVATE,				UIWorldMapOnPostActiveChange);						
	UIMAP(UITYPE_WORLDMAP,		UIMSG_POSTINACTIVATE,			UIWorldMapOnPostActiveChange);						
	UIMAP(UITYPE_WORLDMAP,		UIMSG_ACTIVATE,					UIComponentOnActivate);						
	UIMAP(UITYPE_WORLDMAP,		UIMSG_INACTIVATE,				UIComponentOnInactivate);						

	UIMAP(UITYPE_TRACKER,		UIMSG_PAINT,					UITrackerOnPaint);
	UIMAP(UITYPE_TRACKER,		UIMSG_ACTIVATE,					UIComponentOnActivate);
	UIMAP(UITYPE_TRACKER,		UIMSG_INACTIVATE,				UIComponentOnInactivate);

// vvv Tugboat
	UIMAP(UITYPE_WORLDMAP,		UIMSG_LBUTTONCLICK,	    		UIWorldMapOnLButtonUp);
	UIMAP(UITYPE_WORLDMAP,		UIMSG_MOUSEHOVER,				UIWorldMapOnMouseHover);		
				
	UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name					function pointer
		{ "UIGetHitIndicatorDraw",			UIGetHitIndicatorDraw },
		{ "UIGunModScreenOnActivate",		UIGunModScreenOnActivate },
		{ "UIGunModScreenOnInactivate",		UIGunModScreenOnInactivate },
		{ "UIGunModScreenConfirmClick",		UIGunModScreenOnCancelClick },
		{ "UIGunModScreenCancelClick",		UIGunModScreenOnCancelClick },
		{ "UIGunModImageOnPaint",			UIGunModImageOnPaint },
		{ "UIGunModTooltipOnSetHoverUnit",	UIGunModTooltipOnSetHoverUnit },

		{ "UIAltOverviewMapButtonDown",		UIAltOverviewMapButtonDown },
		{ "UIAltAutomapButtonDown",			UIAltAutomapButtonDown },
		{ "UIAltInventoryButtonDown",		UIAltInventoryButtonDown },
		{ "UIAltSkillsButtonDown",			UIAltSkillsButtonDown },
		{ "UIAltPerksButtonDown",			UIAltPerksButtonDown },
		{ "UIAltOptionsButtonDown",			UIAltOptionsButtonDown },
		{ "UIAltQuestLogButtonDown",		UIAltQuestLogButtonDown},
		{ "UIAltBuddyListButtonDown",		UIAltBuddyListButtonDown },
		{ "UIAltGuildPanelButtonDown",		UIAltGuildPanelButtonDown },
		{ "UIAltPartylistPanelButtonDown",	UIAltPartylistPanelButtonDown },
		{ "UIAltCustomerSupportButtonDown",	UIAltCustomerSupportButtonDown },
		{ "ShiftSkillIconOnPaint",			ShiftSkillIconOnPaint },
		{ "UISkillIconPanelOnMouseHover",	UISkillIconPanelOnMouseHover },
		{ "UISkillIconPanelOnMouseLeave",	UISkillIconPanelOnMouseLeave },
		{ "UIItemIconPanelOnMouseHover",	UIItemIconPanelOnMouseHover },
		{ "UIItemIconPanelOnMouseLeave",	UIItemIconPanelOnMouseLeave },
		{ "RadialMenuBackgroundOnPaint",	RadialMenuBackgroundOnPaint },
		{ "UIRadialMenuOnButtonUp",			UIRadialMenuOnButtonUp	},
		{ "UIRadialMenuOnLButtonDown",		UIRadialMenuOnLButtonDown	},
		{ "UIRadialMenuButtonOnButtonUp",	UIRadialMenuButtonOnButtonUp },
		{ "UIRadialMenuOnPostInactivate",	UIRadialMenuOnPostInactivate },
		{ "UIWorldMapOnPaint",				UIWorldMapOnPaint },
		{ "UIWorldMapOnPostActivate",		UIWorldMapOnPostActivate },
		{ "UIWeaponConfigOnSetFocusStat",	UIWeaponConfigOnSetFocusStat },
		{ "UIEnergyMeterSetVisibility",		UIEnergyMeterSetVisibility },
		{ "UIEnergyMeterResetFadeout",		UIEnergyMeterResetFadeout },
		{ "UITravelMenuWalk",				UITravelMenuWalk },
		{ "UITravelMenuWarp",				UITravelMenuWarp },
		{ "UIZoneComboOnSelect",			UIZoneComboOnSelect },
		{ "UIZoneComboOnActivate",			UIZoneComboOnActivate },
		{ "UIMapZoom",						UIMapZoom },
		{ "UIWorldMapOnMouseHover",			UIWorldMapOnMouseHover },
		{ "UIWorldMapListBoxOnMouseHover",	UIWorldMapListBoxOnMouseHover },
		{ "UIWorldMapListBoxOnLButtonDown",	UIWorldMapListBoxOnLButtonDown },


	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif //!SERVER_VERSION
