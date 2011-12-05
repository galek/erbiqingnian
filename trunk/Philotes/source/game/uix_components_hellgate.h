//----------------------------------------------------------------------------
// uix_components_hellgate.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_COMPONENTS_HELLGATE_H_
#define _UIX_COMPONENTS_HELLGATE_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _UIX_COMPONENTS_COMPLEX_H_
#include "uix_components_complex.h"
#endif


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UITYPE_HUD_TARGETING = NUM_UITYPES_COMPLEX,
	UITYPE_WEAPON_CONFIG,
	UITYPE_GUNMOD,
	UITYPE_MODBOX,
	UITYPE_WORLDMAP,
	UITYPE_TRACKER,
	NUM_UITYPES_HELLGATE,
};

enum TARGET_CURSOR
{
	TCUR_DEFAULT = 0,
	TCUR_HAND,
	TCUR_TALK,
	TCUR_LOCK,
	TCUR_SNIPER,
	TCUR_NONE,

	NUM_TARGET_CURSORS
};
//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
struct UI_HUD_TARGETING : UI_COMPONENT
{
	UI_COMPONENT*		pTargetingCenter;
	UI_COMPONENT*		ppCorner[4];

	UI_TEXTURE_FRAME*	pCursorFrame[NUM_TARGET_CURSORS];
	UI_TEXTURE_FRAME*	m_pAccuracyFrame[UIALIGN_NUM];

	TARGET_CURSOR		eCursor;
	BOOL				bShowBrackets;
	BOOL				m_bForceUpdate;
	DWORD				m_dwAccuracyColor[2];

	// public methods
	UI_HUD_TARGETING(void);
};

struct UI_GUNMOD : UI_COMPONENT
{
	static const int MAX_MOD_LOC_FRAMES = 8;
	UIX_TEXTURE*			m_pIconTexture;
	UI_TEXTURE_FRAME*		m_ppModLocFrame[MAX_MOD_LOC_FRAMES];
	int						m_pnModLocFrameLoc[MAX_MOD_LOC_FRAMES];
	int						m_pnModLocString[MAX_MOD_LOC_FRAMES];
	int						m_nItemSwitchSound;

	// public methods
	UI_GUNMOD(void);
};

struct UI_MODBOX : UI_COMPONENT
{
	int						m_nInvLocation;
	int						m_nInvGridIdx;
	float					m_fCellBorder;
	UI_TEXTURE_FRAME*		m_pLitFrame;
	UI_TEXTURE_FRAME*		m_pIconFrame;

	// public methods
	UI_MODBOX(void);
};

static const int MAX_SKILLS = 128;
static const int SKILLS_PER_PAGE = 16;
struct UI_SKILLPAGE : UI_COMPONENT 
{
	int						m_nCurrentPage;
	int						m_nInvLoc;
	UI_COMPONENT*			m_pLeftArrow;
	UI_COMPONENT*			m_pRightArrow;
	UI_BUTTONEX*			m_pPageButton[MAX_SKILLS / SKILLS_PER_PAGE];

	UI_TEXTURE_FRAME*		m_pPageButtonLitFrame;
	UI_TEXTURE_FRAME*		m_pPageButtonUnlitFrame;

	// public methods
	UI_SKILLPAGE(void);
};							

struct UI_WEAPON_CONFIG : UI_COMPONENT
{
	UI_COMPONENT*			m_pLeftSkillPanel;
	UI_COMPONENT*			m_pRightSkillPanel;
	UI_COMPONENT*			m_pWeaponPanel;
	UI_COMPONENT*			m_pSelectedPanel;
	UI_COMPONENT*			m_pKeyLabel;

	int						m_nHotkeyTag;
	int						m_nInvLocation;
	float					m_fDoubleWeaponFrameW;
	float					m_fDoubleWeaponFrameH;

	UI_TEXTURE_FRAME*		m_pFrame1Weapon;
	UI_TEXTURE_FRAME*		m_pFrame2Weapon;
	UI_TEXTURE_FRAME*		m_pWeaponLitFrame;
	UI_TEXTURE_FRAME*		m_pSkillLitFrame;
	UI_TEXTURE_FRAME*		m_pSelectedFrame;
	UI_TEXTURE_FRAME*		m_pDoubleSelectedFrame;
	UI_TEXTURE_FRAME*		m_pWeaponBkgdFrameR;
	UI_TEXTURE_FRAME*		m_pWeaponBkgdFrameL;
	UI_TEXTURE_FRAME*		m_pWeaponBkgdFrame2;

	// public methods
	UI_WEAPON_CONFIG(void);
};

#define MAX_WORLDMAP_LEVEL_CONNECTIONS	8
struct UI_WORLDMAP_LEVEL
{
	int		m_nLevelID;
	int		m_nRow;
	int		m_nCol;
	int		m_nLabelPosition;
	UI_TEXTURE_FRAME*		m_pFrame;
	UI_TEXTURE_FRAME*		m_pFrameHidden;
	UI_WORLDMAP_LEVEL*		m_ppConnections[MAX_WORLDMAP_LEVEL_CONNECTIONS];

	// public methods
	UI_WORLDMAP_LEVEL(void);
};

struct UI_WORLD_MAP : UI_COMPONENT
{
	UI_WORLDMAP_LEVEL*	m_pLevels;
	UI_TEXTURE_FRAME*	m_pVertConnector;
	UI_TEXTURE_FRAME*	m_pHorizConnector;
	UI_TEXTURE_FRAME*	m_pNESWConnector;
	UI_TEXTURE_FRAME*	m_pNWSEConnector;
	UI_TEXTURE_FRAME*	m_pYouAreHere;
	UI_TEXTURE_FRAME*	m_pPointOfInterest;

	float				m_fColWidth;
	float				m_fRowHeight;
	float				m_fDiagonalAdjustmentX;
	float				m_fDiagonalAdjustmentY;
	UI_COMPONENT*		m_pLevelsPanel;
	UI_COMPONENT*		m_pConnectionsPanel;
	UI_COMPONENT*		m_pIconsPanel;
	DWORD				m_dwConnectionColor;
	DWORD				m_dwStationConnectionColor;

	UI_TEXTURE_FRAME*	m_pNextStoryQuestIcon;

	// vvv Tugboat-specific
	float				m_fOriginalMapWidth;
	float				m_fOriginalMapHeight;
	float				m_fIconOffsetX;
	float				m_fIconOffsetY;
	float				m_fIconScale;
	UI_TEXTURE_FRAME*	m_pTownIcon;
	UI_TEXTURE_FRAME*	m_pDungeonIcon;
	UI_TEXTURE_FRAME*	m_pCaveIcon;
	UI_TEXTURE_FRAME*	m_pCryptIcon;
	UI_TEXTURE_FRAME*	m_pCampIcon;
	UI_TEXTURE_FRAME*	m_pTowerIcon;
	UI_TEXTURE_FRAME*	m_pChurchIcon;
	UI_TEXTURE_FRAME*	m_pCitadelIcon;
	UI_TEXTURE_FRAME*	m_pHengeIcon;
	UI_TEXTURE_FRAME*	m_pFarmIcon;
	UI_TEXTURE_FRAME*	m_pShrineIcon;
	UI_TEXTURE_FRAME*	m_pForestIcon;
	UI_TEXTURE_FRAME*	m_pPartyIsHere;
	UI_TEXTURE_FRAME*	m_pNewArea;
	UI_TEXTURE_FRAME*	m_pLineFrame;

	// public methods
	UI_WORLD_MAP(void);
};

struct UI_TRACKER : UI_COMPONENT
{
	static const int	MAX_TARGETS = 10;
	static const int	MAX_TARGET_FRAMES = 10;

	UI_TEXTURE_FRAME*	m_pTargetFrame[MAX_TARGET_FRAMES];
	UI_TEXTURE_FRAME*	m_pSweeperFrame;
	UI_TEXTURE_FRAME*	m_pCrosshairsFrame;
	UI_TEXTURE_FRAME*	m_pOverlayFrame;
	UI_TEXTURE_FRAME*	m_pRingsFrame;

	int					m_nTargetID[MAX_TARGETS];
	VECTOR				m_vTargetPos[MAX_TARGETS];
	int					m_nTargetFrameType[MAX_TARGETS];
	DWORD				m_dwTargetColor[MAX_TARGETS];
	UI_ANIM_INFO		m_tUpdateAnimInfo;
	float				m_fScale;
	int					m_nSweepPeriodMSecs;
	int					m_nRingsPeriodMSecs;
	int					m_nPingPeriodMSecs;

	float				m_fPingDistanceRads;		// calculated
	float				m_fRadius;
	BOOL				m_bTracking;

	// public methods
	UI_TRACKER(void);
};

//----------------------------------------------------------------------------
// CASTING FUNCTIONS
//----------------------------------------------------------------------------

CAST_FUNC(UITYPE_GUNMOD, UI_GUNMOD, GunMod);
CAST_FUNC(UITYPE_MODBOX, UI_MODBOX, ModBox);
CAST_FUNC(UITYPE_HUD_TARGETING, UI_HUD_TARGETING, HUDTargeting);
CAST_FUNC(UITYPE_WEAPON_CONFIG, UI_WEAPON_CONFIG, WeaponConfig);
CAST_FUNC(UITYPE_WORLDMAP, UI_WORLD_MAP, WorldMap);
CAST_FUNC(UITYPE_TRACKER, UI_TRACKER, Tracker);

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
void UITargetChangeCursor(
	struct UNIT* target,
	TARGET_CURSOR eCursor,
	DWORD dwColor,
	BOOL bShowBrackets,
	DWORD dwColor2 = GFXCOLOR_HOTPINK);

void UIRadialMenuActivate(
	UNIT *pTarget,
	struct INTERACT_MENU_CHOICE *ptInteractChoices,
	int nMenuCount,
	UI_COMPONENT *pComponent = NULL,
	PGUID guid = INVALID_GUID);

void UIQuestTrackerUpdatePos(
	int nID,
	const VECTOR & vector,
	int nType = 0,
	DWORD dwColor = GFXCOLOR_WHITE);

void UIQuestTrackerClearAll(
	void);

BOOL UIQuestTrackerRemove(
	int nID);

void UIQuestTrackerEnable(
	BOOL bTurnOn);

void UIQuestTrackerToggle(
	void);

float RadialClipLine(float x1, float y1, float& x2, float& y2, float radius);

void InitComponentTypesHellgate(UI_XML_COMPONENT *pComponentTypes, UI_XML_ONFUNC*& pUIXmlFunctions, int& nXmlFunctionsSize);

// -- Load functions ----------------------------------------------------------
BOOL UIXmlLoadGunMod		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadModBox		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadActiveSlot	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadHUDTargeting	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadWeaponConfig	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadWorldMap		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadTracker		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);

// -- Free functions ----------------------------------------------------------
void UIComponentFreeGunMod			(UI_COMPONENT* component);
void UIComponentFreeAmmoIndicator	(UI_COMPONENT* component);

// -- Exported message handler functions --------------------------------------
UI_MSG_RETVAL UIGunModScreenOnInactivate	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIUnitNameDisplayOnClick		(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);


#endif