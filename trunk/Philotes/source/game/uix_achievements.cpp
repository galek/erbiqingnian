//----------------------------------------------------------------------------
// uix_achievements.cpp
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
#include "uix_components.h"
#include "uix_components_complex.h"
#include "uix_graphic.h"
#include "uix_scheduler.h"
#include "uix_achievements.h"
#include "player.h"
#include "achievements.h"
#include "performance.h"
#include "units.h"
#include "excel.h"
#include "skills.h"
#include "fontcolor.h"
#include "pstring.h"
#include "dictionary.h"

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAchievementsSetSelected(
	UI_COMPONENT *pComponent,
	int nSelected,
	float fY,
	float fHeight)
{
	GAME* pGame = AppGetCltGame();
	if (!pGame)
		return;
	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return;

	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nSelected);
	ASSERT_RETURN(pAchievement);

	UI_COMPONENT *pHighlightPanel = UIComponentFindChildByName(pComponent, "highlight panel");
	UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent, "name column");
	ASSERT_RETURN(pHighlightPanel);
	ASSERT_RETURN(pNamePanel);

	pHighlightPanel->m_Position.m_fY = (pNamePanel->m_Position.m_fY + fY) - pHighlightPanel->m_pParent->m_Position.m_fY; 
	if( AppIsTugboat() )
	{
		pHighlightPanel->m_Position.m_fY = (pNamePanel->m_Position.m_fY + fY);
	}
	else
	{
		pHighlightPanel->m_fHeight = fHeight;
	}

	pComponent->m_dwData = (DWORD)nSelected;

	UIComponentSetVisible(pHighlightPanel, TRUE);
	UIComponentHandleUIMessage(pHighlightPanel, UIMSG_PAINT, 0, 0);
	UI_COMPONENT *pLabel = UIComponentFindChildByName(pComponent, "detail label");
	// details of Achievement
	if (pLabel)
	{
		WCHAR szText[2048];

		if (c_AchievementGetDetails(pPlayer, pAchievement, szText, arrsize(szText)))
		{
			UILabelSetText(pLabel, szText);
		}
		else
		{
			UILabelSetText(pLabel, L"");
		}
	}
	if( AppIsTugboat() )
	{
		x_AchievementPlayerSelect( pPlayer, nSelected, 0 ); //bit of a hack right now
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sAchievementsSortName( const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	ACHIEVEMENT_DATA *ach1 = *(ACHIEVEMENT_DATA **)a1;
	ACHIEVEMENT_DATA *ach2 = *(ACHIEVEMENT_DATA **)a2;
	ASSERT_RETZERO(ach1);
	ASSERT_RETZERO(ach2);

	const WCHAR *s1 = StringTableGetStringByIndex(ach1->nNameString);
	const WCHAR *s2 = StringTableGetStringByIndex(ach2->nNameString);

	return PStrICmp(s1, s2);
}

static int sAchievementsSortRewardType( const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	ACHIEVEMENT_DATA *ach1 = *(ACHIEVEMENT_DATA **)a1;
	ACHIEVEMENT_DATA *ach2 = *(ACHIEVEMENT_DATA **)a2;
	ASSERT_RETZERO(ach1);
	ASSERT_RETZERO(ach2);

	const WCHAR *s1 = StringTableGetStringByIndex(ach1->nRewardTypeString);
	const WCHAR *s2 = StringTableGetStringByIndex(ach2->nRewardTypeString);

	return PStrICmp(s1, s2);
}



static int sAchievementsSortCondition( const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	ACHIEVEMENT_DATA *ach1 = *(ACHIEVEMENT_DATA **)a1;
	ACHIEVEMENT_DATA *ach2 = *(ACHIEVEMENT_DATA **)a2;
	ASSERT_RETZERO(ach1);
	ASSERT_RETZERO(ach2);

	const WCHAR *s1 = StringTableGetStringByIndex(ach1->nDescripString);
	const WCHAR *s2 = StringTableGetStringByIndex(ach2->nDescripString);

	return PStrICmp(s1, s2);
}

static int sAchievementsSortProgress( void * pContext, const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	ASSERT_RETZERO(pContext);
	ACHIEVEMENT_DATA *ach1 = *(ACHIEVEMENT_DATA **)a1;
	ACHIEVEMENT_DATA *ach2 = *(ACHIEVEMENT_DATA **)a2;
	ASSERT_RETZERO(ach1);
	ASSERT_RETZERO(ach2);

	UNIT *pPlayer = (UNIT *)pContext;

	ASSERT_RETFALSE(ach1->nCompleteNumber != 0);
	ASSERT_RETFALSE(ach2->nCompleteNumber != 0);

	int nProgress1 = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, ach1->nIndex);
	float fPercent1 = (float)(nProgress1) / (float)ach1->nCompleteNumber;

	int nProgress2 = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, ach2->nIndex);
	float fPercent2 = (float)(nProgress2) / (float)ach2->nCompleteNumber;

	if (fPercent1 > fPercent2)
		return -1;
	else 
		return 1;
}

static int sAchievementsSortPoints( const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	ACHIEVEMENT_DATA *ach1 = *(ACHIEVEMENT_DATA **)a1;
	ACHIEVEMENT_DATA *ach2 = *(ACHIEVEMENT_DATA **)a2;
	ASSERT_RETZERO(ach1);
	ASSERT_RETZERO(ach2);

	if (ach1->nRewardAchievementPoints > ach2->nRewardAchievementPoints)
		return -1;
	else
		return 1;
}


UI_MSG_RETVAL UIAchievementSlotsOnPaint(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(pComponent))
	{
		return UIMSG_RET_HANDLED;
	}
	UIComponentOnPaint(pComponent, msg, wParam, lParam);
	UNIT *pPlayer = UIGetControlUnit();
	if (!pPlayer)
		return UIMSG_RET_HANDLED;

	static const float scfBordersize = 5.0f;

	UI_COMPONENT *pSlot = UIComponentFindChildByName(pComponent, "achievement slot");

	ASSERT_RETVAL(pSlot, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(pComponent);


	// we also need to display the slots that we can drop achievements into -

	int nSlots = AchievementGetSlotCount();
	for( int i = 0; i < nSlots; i++ )
	{
		UI_DRAWSKILLICON_PARAMS tParams(pComponent, UI_RECT(), UI_POSITION(), -1, pSlot->m_pFrame, NULL, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat() /* Tugboat doesn't do the small icons here*/ );
		UI_POSITION pos = UI_POSITION( i * 32.0f * g_UI.m_fUIScaler, 10);

		tParams.pos = pos;
		if( AchievementSlotIsValid( pPlayer, i ) )
		{
			const ACHIEVEMENT_DATA * pAchievementData = AchievementGetBySlotIndex( pPlayer, i );	
			if( pAchievementData &&
				pAchievementData->nRewardSkill != INVALID_ID )
			{
				tParams.nSkill = pAchievementData->nRewardSkill;
			}
			tParams.byAlpha = 255;
		}
		else
		{
			tParams.byAlpha = 100;
		}
		tParams.rect.m_fX1 = pos.m_fX;
		tParams.rect.m_fY1 = pos.m_fY;													
		tParams.rect.m_fX2 = tParams.rect.m_fX1 + 28 * g_UI.m_fUIScaler;
		tParams.rect.m_fY2 = tParams.rect.m_fY1 + 28 * g_UI.m_fUIScaler;
		tParams.fSkillIconHeight = 28 * g_UI.m_fUIScaler;
		tParams.fSkillIconWidth = 28 * g_UI.m_fUIScaler;
		UIDrawSkillIcon(tParams);		
	}

	return UIMSG_RET_HANDLED;
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementSlotOnPaint(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT *pPlayer = UIGetControlUnit();
	if (!pPlayer)
		return UIMSG_RET_HANDLED;

	int nSlot = (int)pComponent->m_dwParam;
	ASSERT_RETVAL( nSlot >= 0 && nSlot < AchievementGetSlotCount(), UIMSG_RET_HANDLED );		

	float fBorder = (float)pComponent->m_dwParam2;

	BOOL bValid = AchievementSlotIsValid( pPlayer, nSlot );
	if (bValid)
	{
		pComponent->m_dwColor = UI_MAKE_COLOR(255, pComponent->m_dwColor);
	}
	else
	{
		pComponent->m_dwColor = UI_MAKE_COLOR(100, pComponent->m_dwColor);
	}

	UIComponentRemoveAllElements(pComponent);
	UIComponentAddFrame(pComponent);

	const ACHIEVEMENT_DATA * pAchievementData = AchievementGetBySlotIndex( pPlayer, nSlot );	
	if( pAchievementData &&
		pAchievementData->nRewardSkill != INVALID_ID )
	{
		UI_DRAWSKILLICON_PARAMS tParams(pComponent, UI_RECT(), UI_POSITION(), -1, NULL, NULL, NULL, NULL, bValid ? 255 : 100, FALSE, FALSE, !AppIsTugboat() /* Tugboat doesn't do the small icons here*/ );
		tParams.nSkill = pAchievementData->nRewardSkill;
		tParams.pos = UI_POSITION( fBorder * g_UI.m_fUIScaler, fBorder * g_UI.m_fUIScaler);
		tParams.fSkillIconHeight = (pComponent->m_fHeight - (2.0f * fBorder)) * g_UI.m_fUIScaler;
		tParams.fSkillIconWidth = (pComponent->m_fWidth - (2.0f * fBorder)) * g_UI.m_fUIScaler;
		tParams.rect.m_fX1 = tParams.pos.m_fX;
		tParams.rect.m_fY1 = tParams.pos.m_fY;													
		tParams.rect.m_fX2 = tParams.rect.m_fX1 + tParams.fSkillIconWidth;
		tParams.rect.m_fY2 = tParams.rect.m_fY1 + tParams.fSkillIconHeight;
		tParams.bNegateOffsets = TRUE;
		UIDrawSkillIcon(tParams);		
	}

	return UIMSG_RET_HANDLED;
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsOnPaintTugboat(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam,
	UNIT *pPlayer )
{
	if (!UIComponentGetActive(pComponent))
	{
		return UIMSG_RET_HANDLED;
	}
	static const float scfBordersize = 3.0f;

	GAME *pGame = AppGetCltGame();
	UI_COMPONENT *pSkillPanel = UIComponentFindChildByName(pComponent, "skill column");
	UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent, "name column");
	UI_COMPONENT *pParentFrame = pNamePanel->m_pParent;	
	UI_COMPONENT *pComp = UIComponentFindChildByName(pParentFrame->m_pParent, "achievement list scrollbar");
	UI_COMPONENT *pSlot = UIComponentFindChildByName(pComponent, "achievement slot");



	ASSERT_RETVAL(pSlot, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pNamePanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pParentFrame, UIMSG_RET_NOT_HANDLED);		
	ASSERT_RETVAL(pComp, UIMSG_RET_NOT_HANDLED);
	

	UI_SCROLLBAR *pScrollbar = UICastToScrollBar(pComp);	

	UIComponentRemoveAllElements(pSkillPanel);	
	UIComponentRemoveAllElements(pNamePanel);	

	UI_RECT rectNamePanel =			UIComponentGetRect(pNamePanel);
	

	UI_RECT rectNamePanelFull =		rectNamePanel;	
	
	rectNamePanel.m_fX1 += scfBordersize;		rectNamePanel.m_fX2 -= scfBordersize;	

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pComponent);
	int nFontSize = UIComponentGetFontSize(pComponent);
	const static int szTextSize( 256 );
	WCHAR szText[szTextSize];
	UI_ELEMENT_TEXT *pElement = NULL;
	
	int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);

	ACHIEVEMENT_DATA **ppAchievements = (ACHIEVEMENT_DATA **)MALLOCZ(NULL, sizeof(ACHIEVEMENT_DATA *) * (nCount+1));
	int nRevealed = 0;
	for (int iAchievement = 0; iAchievement < nCount; ++iAchievement )
	{
		ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
#if ISVERSION(DEVELOPMENT)
		if (pAchievement )
		{
			ppAchievements[nRevealed++] = pAchievement;
		}
#else
		if (pAchievement && c_AchievementIsRevealed(pPlayer, pAchievement))
		{
			ppAchievements[nRevealed++] = pAchievement;
		}
#endif
	}

	qsort(ppAchievements, nRevealed, sizeof(ACHIEVEMENT_DATA *), sAchievementsSortName);
	
	float nSpacer( (float)nFontSize * g_UI.m_fUIScaler + 1.0f  ) ;
	float fY = 0.0f;
	for (int i=0; i < nRevealed; i++)
	{
		ACHIEVEMENT_DATA *pAchievement = ppAchievements[i];
		DWORD dwColor = GFXCOLOR_WHITE;//GFXCOLOR_YELLOW;
		BOOL bIsComplete( x_AchievementIsComplete( pPlayer, pAchievement ) );
		if( bIsComplete )
		{
			dwColor = GFXCOLOR_WHITE;			
		}
		else if( UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex) > 0 )
		{
			dwColor = GFXCOLOR_GRAY;
		}
		else
		{
			dwColor = GFXCOLOR_DKGRAY;
		}


		// Title of Achievement
		if (c_AchievementGetName(pPlayer, pAchievement, szText, szTextSize) && szText[0])
		{
			PStrPrintf( szText, szTextSize, L"%s - ", szText );
			int printable = 0;			
			UICountPrintableChar(szText, &printable);			
			c_AchievementGetDescription(pPlayer, pAchievement, &szText[printable], szTextSize - printable);
			if( pAchievement->nCompleteNumber > 1 )
			{
				int nProgress = UnitGetStat(pPlayer, STATS_ACHIEVEMENT_PROGRESS, pAchievement->nIndex);
				PStrPrintf( szText, szTextSize, L"%s - %i/%i ", szText, nProgress, pAchievement->nCompleteNumber );
			}
			pElement = UIComponentAddTextElement(pNamePanel, NULL, pFont, nFontSize, szText, UI_POSITION(scfBordersize, fY), dwColor, &rectNamePanel, UIALIGN_TOPLEFT, &UI_SIZE(rectNamePanel.Width() * g_UI.m_fUIScaler, (float)nFontSize * g_UI.m_fUIScaler + 1.0f ) ); 
			if (pElement)
				pElement->m_qwData = (QWORD)pAchievement->nIndex;
		}
		// The skill icon, if applicable
		if( pAchievement->nRewardSkill != INVALID_ID )
		{
			UI_DRAWSKILLICON_PARAMS tParams(pSkillPanel, UI_RECT(), UI_POSITION(), -1, pSlot->m_pFrame, NULL, NULL, NULL, 255, FALSE, FALSE, !AppIsTugboat() /* Tugboat doesn't do the small icons here*/ );
			UI_POSITION pos = UI_POSITION(0, fY);
			tParams.bGreyout = !bIsComplete;			
			tParams.pos = pos;
			tParams.nSkill = pAchievement->nRewardSkill;
			tParams.rect.m_fX1 = 0;
			tParams.rect.m_fY1 = 0;													
			tParams.rect.m_fX2 = tParams.rect.m_fX1 + 32 * g_UI.m_fUIScaler;
			tParams.rect.m_fY2 = tParams.rect.m_fY1 + 32000 * g_UI.m_fUIScaler;
			tParams.fSkillIconHeight = 28 * g_UI.m_fUIScaler;
			tParams.fSkillIconWidth = 28 * g_UI.m_fUIScaler;
			UIDrawSkillIcon(tParams);

		}

		// Description
		//if (c_AchievementGetDescription(pPlayer, pAchievement, szText, szTextSize) && szText[0])
		if (c_AchievementGetDetails(pPlayer, pAchievement, szText, szTextSize) && szText[0])
		{			
			if( bIsComplete )
				dwColor = GFXCOLOR_YELLOW;
			else
				dwColor = GFXCOLOR_DKGRAY;

			pElement = UIComponentAddTextElement(pNamePanel, NULL, pFont, nFontSize, szText, UI_POSITION(10 * g_UI.m_fUIScaler, fY + nSpacer), dwColor, &rectNamePanel, UIALIGN_TOPLEFT, &UI_SIZE(rectNamePanel.Width() * 2, (float)nFontSize + 1.0f)); 			
			if (pElement)
				pElement->m_qwData = (QWORD)pAchievement->nIndex;
		}
		fY += nSpacer * 2.75f ;
	}

	pScrollbar->m_fMax = MAX(0.0f, fY - pNamePanel->m_fHeight);
	pScrollbar->m_fScrollVertMax = pScrollbar->m_fMax;
		
	
	if (pNamePanel->m_pFrame)
	{
		UIComponentAddElement(pNamePanel, UIComponentGetTexture(pNamePanel), pNamePanel->m_pFrame, UI_POSITION(), pNamePanel->m_dwColor, &rectNamePanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pNamePanel->m_fWidth, fY));
	}
	

	FREE(NULL, ppAchievements);


	return UIMSG_RET_HANDLED;	
}

UI_MSG_RETVAL UIAchievementsOnPaint(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentOnPaint(pComponent, msg, wParam, lParam);
	UNIT *pPlayer = UIGetControlUnit();
	if (!pPlayer)
		return UIMSG_RET_HANDLED;

	TIMER_START_NEEDED("Acheivements panel paint");

	if( AppIsTugboat() )
	{
		return UIAchievementsOnPaintTugboat( pComponent, msg, wParam, lParam, pPlayer );
	}


	static const float scfBordersize = 5.0f;

	GAME *pGame = AppGetCltGame();
	UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent, "name column");
	UI_COMPONENT *pConditionPanel = UIComponentFindChildByName(pComponent, "condition column");
	UI_COMPONENT *pProgressPanel = UIComponentFindChildByName(pComponent, "progress column");
	UI_COMPONENT *pPointsPanel = UIComponentFindChildByName(pComponent, "points column");
	UI_COMPONENT *pIconPanel = UIComponentFindChildByName(pComponent, "icon panel");
	UI_COMPONENT *pComp = UIComponentFindChildByName(pComponent, "achievement list scrollbar");
	
	ASSERT_RETVAL(pNamePanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pConditionPanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pProgressPanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pPointsPanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pIconPanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pComp, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollbar = UICastToScrollBar(pComp);

	UIComponentRemoveAllElements(pNamePanel);
	UIComponentRemoveAllElements(pConditionPanel);
	UIComponentRemoveAllElements(pProgressPanel);
	UIComponentRemoveAllElements(pPointsPanel);
	UIComponentRemoveAllElements(pIconPanel);


	UI_RECT rectNamePanel =			UIComponentGetRect(pNamePanel);
	UI_RECT rectConditionPanel =	UIComponentGetRect(pConditionPanel);
	UI_RECT rectProgressPanel =		UIComponentGetRect(pProgressPanel);
	UI_RECT rectPointsPanel =		UIComponentGetRect(pPointsPanel);
	UI_RECT rectIconPanel =			UIComponentGetRect(pIconPanel);

	UI_RECT rectNamePanelFull =		rectNamePanel;
	UI_RECT rectConditionPanelFull =rectConditionPanel;
	UI_RECT rectProgressPanelFull =	rectProgressPanel;
	UI_RECT rectPointsPanelFull =	rectPointsPanel;
	UI_RECT rectIconPanelFull =		rectIconPanel;

	rectNamePanel.m_fX1 += scfBordersize;		rectNamePanel.m_fX2 -= scfBordersize;
	rectConditionPanel.m_fX1 += scfBordersize;	rectConditionPanel.m_fX2 -= scfBordersize;
	rectProgressPanel.m_fX1 += scfBordersize;	rectProgressPanel.m_fX2 -= scfBordersize;
	rectPointsPanel.m_fX1 += scfBordersize;		rectPointsPanel.m_fX2 -= scfBordersize;
	rectIconPanelFull.m_fX1 += scfBordersize;	rectIconPanelFull.m_fX2 -= scfBordersize;

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pComponent);
	int nFontSize = UIComponentGetFontSize(pComponent);
	WCHAR szText[256];
	UI_ELEMENT_TEXT *pElement = NULL;
	float fY = 0.0f;
	int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);

	ACHIEVEMENT_DATA **ppAchievements = (ACHIEVEMENT_DATA **)MALLOCZ(NULL, sizeof(ACHIEVEMENT_DATA *) * (nCount+1));

	int nRevealed = 0;
	for (int iAchievement = 0; iAchievement < nCount; ++iAchievement )
	{
		ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);

		if (pAchievement && c_AchievementIsRevealed(pPlayer, pAchievement))
		{
			ppAchievements[nRevealed++] = pAchievement;
		}
	}

	if		(UILabelIsSelected(pComponent, "name column label"))		qsort(ppAchievements, nRevealed, sizeof(ACHIEVEMENT_DATA *), sAchievementsSortName);
	else if (UILabelIsSelected(pComponent, "condition column label"))	qsort(ppAchievements, nRevealed, sizeof(ACHIEVEMENT_DATA *), sAchievementsSortCondition);
	else if (UILabelIsSelected(pComponent, "progress column label"))	qsort_s(ppAchievements, nRevealed, sizeof(ACHIEVEMENT_DATA *), sAchievementsSortProgress, (void *)pPlayer);
	else if (UILabelIsSelected(pComponent, "points column label"))		qsort(ppAchievements, nRevealed, sizeof(ACHIEVEMENT_DATA *), sAchievementsSortPoints);

	for (int i=0; i < nRevealed; i++)
	{
		ACHIEVEMENT_DATA *pAchievement = ppAchievements[i];

		DWORD dwColor = GFXCOLOR_GRAY;
		float fLineSize = (float)nFontSize;

		if (pAchievement->nRewardSkill != INVALID_ID)
		{
			const SKILL_DATA *pSkillData = SkillGetData( pGame, pAchievement->nRewardSkill );
			ASSERT( pSkillData );

			const SKILLTAB_DATA *pSkillTabData = pSkillData->nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pSkillData->nSkillTab) : NULL;
			ASSERT(pSkillTabData);

			UIX_TEXTURE *pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
			ASSERT( pSkillTexture );

			UI_TEXTURE_FRAME* pSkillIcon = (UI_TEXTURE_FRAME*)StrDictionaryFind(pSkillTexture->m_pFrames, pSkillData->szSmallIcon);
			ASSERT(pSkillIcon);

			UI_GFXELEMENT *pElement = UIComponentAddElement(pIconPanel, pSkillTexture, pSkillIcon, UI_POSITION(0.0f, fY), GFXCOLOR_WHITE, &rectIconPanel, 0, 1.0f, 1.0f, &UI_SIZE(pIconPanel->m_fWidth, pIconPanel->m_fWidth));			
			if (pElement)
			{
				pElement->m_qwData = (QWORD)pAchievement->nRewardSkill;
				pElement->m_fXOffset = 0.0f;
				pElement->m_fYOffset = 0.0f;
				fLineSize = MAX(fLineSize, pElement->m_fHeight);
			}
		}

		// Title of Achievement
		if (c_AchievementGetName(pPlayer, pAchievement, szText, arrsize(szText)) && szText[0])
		{
			pElement = UIComponentAddTextElement(pNamePanel, NULL, pFont, nFontSize, szText, UI_POSITION(scfBordersize, fY), dwColor, &rectNamePanel, UIALIGN_LEFT, &UI_SIZE(rectNamePanel.Width(), fLineSize + 1.0f)); 
			if (pElement)
				pElement->m_qwData = (QWORD)pAchievement->nIndex;
		}

		// Conditions of Achievement
		if (c_AchievementGetDescription(pPlayer, pAchievement, szText, arrsize(szText)) && szText[0])
		{
			pElement = UIComponentAddTextElement(pConditionPanel, NULL, pFont, nFontSize, szText, UI_POSITION(scfBordersize, fY), dwColor, &rectConditionPanel, UIALIGN_LEFT, &UI_SIZE(rectConditionPanel.Width(), fLineSize + 1.0f)); 
			if (pElement)
				pElement->m_qwData = (QWORD)pAchievement->nIndex;
		}

		// Progress on Achievement
		if (c_AchievementGetProgress(pPlayer, pAchievement, szText, arrsize(szText)) && szText[0])
		{
			pElement = UIComponentAddTextElement(pProgressPanel, NULL, pFont, nFontSize, szText, UI_POSITION(scfBordersize, fY), dwColor, &rectProgressPanel, UIALIGN_CENTER, &UI_SIZE(rectProgressPanel.Width(), fLineSize + 1.0f)); 
			if (pElement)
				pElement->m_qwData = (QWORD)pAchievement->nIndex;
		}

		// Reward for Achievement
		if (c_AchievementGetAP(pPlayer, pAchievement, szText, arrsize(szText), FALSE) && szText[0])
		{
			pElement = UIComponentAddTextElement(pPointsPanel, NULL, pFont, nFontSize, szText, UI_POSITION(scfBordersize, fY), dwColor, &rectPointsPanel, UIALIGN_CENTER, &UI_SIZE(rectPointsPanel.Width(), fLineSize + 1.0f)); 
			if (pElement)
				pElement->m_qwData = (QWORD)pAchievement->nIndex;
		}

		fY += fLineSize + 1.0f;
	}

	if (pNamePanel->m_pFrame)
	{
		UIComponentAddElement(pNamePanel, UIComponentGetTexture(pNamePanel), pNamePanel->m_pFrame, UI_POSITION(), pNamePanel->m_dwColor, &rectNamePanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pNamePanel->m_fWidth, fY));
	}
	if (pConditionPanel->m_pFrame)
	{
		UIComponentAddElement(pConditionPanel, UIComponentGetTexture(pConditionPanel), pConditionPanel->m_pFrame, UI_POSITION(), pConditionPanel->m_dwColor, &rectConditionPanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pConditionPanel->m_fWidth, fY));
	}

	if (pProgressPanel->m_pFrame)
	{
		UIComponentAddElement(pProgressPanel, UIComponentGetTexture(pProgressPanel), pProgressPanel->m_pFrame, UI_POSITION(), pProgressPanel->m_dwColor, &rectProgressPanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pProgressPanel->m_fWidth, fY));
	}

	if (pPointsPanel->m_pFrame)
	{
		UIComponentAddElement(pPointsPanel, UIComponentGetTexture(pPointsPanel), pPointsPanel->m_pFrame, UI_POSITION(), pPointsPanel->m_dwColor, &rectPointsPanel, FALSE, 1.0f, 1.0f, &UI_SIZE(pPointsPanel->m_fWidth, fY));
	}

	FREE(NULL, ppAchievements);

	pScrollbar->m_fMax = MAX(0.0f, fY - pNamePanel->m_fHeight);
	pScrollbar->m_fScrollVertMax = pScrollbar->m_fMax;
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsScrollBarOnScroll(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pComponent);

	UIScrollBarSetValue(pScrollBar, pScrollBar->m_ScrollPos.m_fY);

	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	if( AppIsHellgate() )
	{
		UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent->m_pParent, "name column");
		UI_COMPONENT *pConditionPanel = UIComponentFindChildByName(pComponent->m_pParent, "condition column");
		UI_COMPONENT *pProgressPanel = UIComponentFindChildByName(pComponent->m_pParent, "progress column");
		UI_COMPONENT *pPointsPanel = UIComponentFindChildByName(pComponent->m_pParent, "points column");
		UI_COMPONENT *pIconPanel = UIComponentFindChildByName(pComponent->m_pParent, "icon panel");
		UI_COMPONENT *pHighlightPanel = UIComponentFindChildByName(pComponent->m_pParent, "highlight panel");

		ASSERT_RETVAL(pNamePanel, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(pConditionPanel, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(pProgressPanel, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(pPointsPanel, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(pIconPanel, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(pHighlightPanel, UIMSG_RET_NOT_HANDLED);

		pNamePanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		pConditionPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		pProgressPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		pPointsPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		pIconPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		pHighlightPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;

		UISetNeedToRender(pComponent->m_pParent);
	}
	else
	{
		UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent->m_pParent, "name column");
		UI_COMPONENT *pSkillPanel = UIComponentFindChildByName(pComponent->m_pParent, "skill column");

		ASSERT_RETVAL(pNamePanel, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(pSkillPanel, UIMSG_RET_NOT_HANDLED);

		pNamePanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		pSkillPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;

		UISetNeedToRender(pComponent->m_pParent);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementColumnOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pComponent);
	y += UIComponentGetScrollPos(pComponent).m_fY;

	x -= pos.m_fX;
	y -= pos.m_fY;
	
	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(pComponent->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pComponent->m_bNoScale);

	UI_GFXELEMENT *pChild = pComponent->m_pGfxElementFirst;
	while(pChild)
	{
		if ( UIElementCheckBounds(pChild, logx, logy ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID )
		{
			if( AppIsHellgate() )
			{
				sAchievementsSetSelected(pComponent->m_pParent, (int)pChild->m_qwData, pChild->m_Position.m_fY, pChild->m_fHeight);
			}
			else
			{
				if( pChild->m_Position.m_fX >= 10.0f )
				{					
					sAchievementsSetSelected(pComponent->m_pParent, (int)pChild->m_qwData, pChild->m_Position.m_fY - 13, pChild->m_fHeight);
				}
				else
				{
					sAchievementsSetSelected(pComponent->m_pParent, (int)pChild->m_qwData, pChild->m_Position.m_fY, pChild->m_fHeight);
				}
			}
			//UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_PAINT, 0, 0);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		pChild = pChild->m_pNextElement;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementIconColumnOnLButtonDown(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(pComponent))
		return UIMSG_RET_NOT_HANDLED;

	float x = 0.0f;	float y = 0.0f;
	UI_POSITION posComponent;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentCheckBounds(pComponent, x, y, &posComponent))
		return UIMSG_RET_NOT_HANDLED;

	UNIT *pPlayer = UIGetControlUnit();
	if (!pPlayer)
		return UIMSG_RET_NOT_HANDLED;

	y += UIComponentGetScrollPos(pComponent).m_fY;

	x -= posComponent.m_fX;
	y -= posComponent.m_fY;
	
	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(pComponent->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(pComponent->m_bNoScale);

	UI_GFXELEMENT *pChild = pComponent->m_pGfxElementFirst;
	while (pChild)
	{
		int nSkillID = (int)pChild->m_qwData;
		if ( UIElementCheckBounds(pChild, logx, logy ) && 
			nSkillID != INVALID_ID &&
			x_AchievementIsCompleteBySkillID(pPlayer, nSkillID))
		{
			UISetCursorSkill(nSkillID, TRUE);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		pChild = pChild->m_pNextElement;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsOnActivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eReturn = UIComponentOnActivate(pComponent, msg, wParam, lParam);
	if (ResultIsHandled(eReturn))
	{
		pComponent->m_dwData = (DWORD)INVALID_LINK;
		UI_COMPONENT *pLabel = UIComponentFindChildByName(pComponent, "detail label");
		if (pLabel)
		{
			UILabelSetText(pLabel, L"");
		}
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}

	return eReturn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsOnPostActivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if( AppIsTugboat() )
	{

		UI_COMPONENT *pLevelUp = UIComponentGetByEnum(UICOMP_ACHIEVEMENTUPPANEL);
		if ( pLevelUp )
		{
			UIComponentThrobEnd( pLevelUp, 0, 0, 0 );
			UIComponentSetVisible( pLevelUp, FALSE );
		}
		return UIMSG_RET_NOT_HANDLED;
	}

	UIStopAllSkillRequests();
	c_PlayerClearAllMovement(AppGetCltGame());

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sRenderAchievements(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	
	int nComponentID = (int)event_data.m_Data1;
	UI_COMPONENT* pComponent = UIComponentGetById( nComponentID );
	if( pComponent )
	{
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsOnProgressChange(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	WORD wStat = STAT_GET_STAT(lParam);
	DWORD dwParam = STAT_GET_PARAM(lParam);

	int nAchievement = (int)dwParam;

	GAME *pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	UNIT *pPlayer = GameGetControlUnit(pGame);
	ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

	int nVal = UnitGetStat(pPlayer, wStat, dwParam);
	ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, nAchievement);
	ASSERT_RETVAL(pAchievement, UIMSG_RET_NOT_HANDLED);

	if (nVal == pAchievement->nCompleteNumber)
	{
		WCHAR szReward[256];
		WCHAR szName[256];
		// Reward for Achievement
		c_AchievementGetName(pPlayer, pAchievement, szName, arrsize(szName));
		c_AchievementGetAP(pPlayer, pAchievement, szReward, arrsize(szReward), TRUE);

//		UIShowQuickMessage(szName, 4.0f, szReward);
		UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_ACHIEVEMENT_COMPLETE_NOTIFY);
		if (pComponent)
		{
			UIComponentActivate(pComponent, TRUE);
			UI_COMPONENT *pLabel = UIComponentFindChildByName(pComponent, "achievement name");
			if (pLabel)
			{
				UILabelSetText(pLabel, szName);
			}
			pLabel = UIComponentFindChildByName(pComponent, "achievement reward");
			if (pLabel)
			{
				UILabelSetText(pLabel, szReward);
			}
			UIComponentActivate(pComponent, FALSE, 5000);
		}
	}

	if (UIComponentGetActive(pComponent))
	{
		//lets mark this dirty instead of updating it on the stat change. In theory you could
		//have 500 achievements and say 50 of them need updating this render - 50*500 is the number
		//of lines it's actually going to render that frame.
		if ( !UnitHasTimedEvent( pPlayer, sRenderAchievements ) )
		{
			UnitRegisterEventTimed( pPlayer, sRenderAchievements, &EVENT_DATA( UIComponentGetId(pComponent) ), GAME_TICKS_PER_SECOND * 2 ); //ok going to repaint it after 3 seconds
		}
		//UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	}
	
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsScrollBarOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UICursorGetActive())
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	if (AppIsHellgate() || ( UIComponentCheckBounds(pScrollBar) ||
		(pScrollBar->m_pScrollControl && UIComponentCheckBounds(pScrollBar->m_pScrollControl))))
	{
		pScrollBar->m_ScrollPos.m_fY += (int)lParam > 0 ? -pScrollBar->m_fWheelScrollIncrement : pScrollBar->m_fWheelScrollIncrement;
		pScrollBar->m_ScrollPos.m_fY = PIN(pScrollBar->m_ScrollPos.m_fY, pScrollBar->m_fMin, pScrollBar->m_fMax);
		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, lParam, FALSE);
		UISetNeedToRender(pScrollBar);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsOnLButtonDown(
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

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( component->m_pParent && 
		!UIComponentCheckBounds(component->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	x -= pos.m_fX;
	y -= pos.m_fY;

	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(component->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(component->m_bNoScale);
	float fScrollOffsetY = UIComponentGetScrollPos(component).m_fY;
	if( component->m_pParent )
	{
		x -= component->m_Position.m_fX;
		logx -= component->m_Position.m_fX;
		fScrollOffsetY += UIComponentGetScrollPos(component->m_pParent).m_fY;
		logy += fScrollOffsetY;
	}
	int nSkillID = INVALID_LINK;

	UI_GFXELEMENT *pChild = component->m_pGfxElementFirst;
	while(pChild)
	{

		if ( UIElementCheckBounds(pChild, logx, logy ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID &&
			(pChild->m_qwData >> 16 == 0))
		{
			if ( (AppIsHellgate() ||
				pChild->m_fZDelta != 0) ||
				AppIsTugboat() )
			{
				skillpos.m_fX = pChild->m_Position.m_fX;
				skillpos.m_fY = pChild->m_Position.m_fY;

				nSkillID = (int)(pChild->m_qwData & 0xFFFF) ;
			}
		}
		pChild = pChild->m_pNextElement;
	}

	if (nSkillID != INVALID_LINK )
	{
		//in the ghettooooooooo
		if( x_AchievementIsCompleteBySkillID( UIGetControlUnit(), nSkillID ) )
		{
			UISetCursorSkill(nSkillID, TRUE);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementSlotsOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	int nCursorSkillID = UIGetCursorSkill();
	int nAchievementID = INVALID_ID;

	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	GAME* pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return UIMSG_RET_NOT_HANDLED;


	float x, y;
	UIGetCursorPosition(&x, &y);
	float OriginalX = x;
	float OriginalY = y;
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( component->m_pParent && 
		!UIComponentCheckBounds(component->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	x -= pos.m_fX;
	y -= pos.m_fY;

	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(component->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(component->m_bNoScale);
	float fScrollOffsetY = UIComponentGetScrollPos(component).m_fY;
	if( component->m_pParent )
	{
		x -= component->m_Position.m_fX;
		logx -= component->m_Position.m_fX;
		fScrollOffsetY += UIComponentGetScrollPos(component->m_pParent).m_fY;
		logy += fScrollOffsetY;
	}
	int nSkillID = INVALID_LINK;

	int nSlotID = INVALID_ID;

	UI_COMPONENT *pGhost = component->m_pFirstChild;
	while( pGhost )
	{
		if( pGhost->m_dwParam > 0 &&
			UIComponentCheckBounds( pGhost, OriginalX, OriginalY ) )
		{
			nSlotID = (int)pGhost->m_dwParam - 1;
			break;
		}
		pGhost = pGhost->m_pNextSibling;
	}
	if( !AchievementSlotIsValid( pPlayer, nSlotID ) )
	{
		nSlotID = INVALID_ID;
	}

	UI_GFXELEMENT *pChild = component->m_pGfxElementFirst;
	while(pChild)
	{

		if ( UIElementCheckBounds(pChild, logx, logy ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID &&
			(pChild->m_qwData >> 16 == 0))
		{
			skillpos.m_fX = pChild->m_Position.m_fX;
			skillpos.m_fY = pChild->m_Position.m_fY;

			nSkillID = (int)(pChild->m_qwData & 0xFFFF) ;
		}
		pChild = pChild->m_pNextElement;
	}

	if (nCursorSkillID != INVALID_ID && nSlotID != INVALID_ID)
	{
		int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);

		// find out which achievement this skill goes to ( if it does indeed )
		for (int iAchievement = 0; iAchievement < nCount; ++iAchievement )
		{
			ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
			if( pAchievement->nRewardSkill == nCursorSkillID )
			{
				nAchievementID = iAchievement;
				break;
			}
		}

		// now we need to know what slot we're clicking on
		x_AchievementPlayerSelect( pPlayer, nAchievementID, nSlotID ); //bit of a hack right now
		UISetCursorSkill(INVALID_ID, FALSE);

	}
	if (nSkillID != INVALID_LINK )
	{
		if( nCursorSkillID == INVALID_ID &&
			nSlotID != INVALID_ID)
		{
			x_AchievementPlayerSelect( pPlayer, INVALID_ID, nSlotID ); //bit of a hack right now
		}
		UISetCursorSkill(nSkillID, TRUE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementSlotOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	int nCursorSkillID = UIGetCursorSkill();
	int nAchievementID = INVALID_ID;

	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	GAME* pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return UIMSG_RET_NOT_HANDLED;

	if (!UIComponentGetActive(component) ||
		!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nSlotID = (int)component->m_dwParam;

	if( !AchievementSlotIsValid( pPlayer, nSlotID ) )
	{
		nSlotID = INVALID_ID;
	}

	if (nSlotID == INVALID_ID)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (nCursorSkillID != INVALID_ID)
	{
		int nCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);

		// find out which achievement this skill goes to ( if it does indeed )
		for (int iAchievement = 0; iAchievement < nCount; ++iAchievement )
		{
			ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAchievement);
			if( pAchievement->nRewardSkill == nCursorSkillID )
			{
				nAchievementID = iAchievement;
				break;
			}
		}

		// now we need to know what slot we're clicking on
		x_AchievementPlayerSelect( pPlayer, nAchievementID, nSlotID ); //bit of a hack right now
		UISetCursorSkill(INVALID_ID, FALSE);

	}
	else
	{
		const ACHIEVEMENT_DATA * pAchievementData = AchievementGetBySlotIndex( pPlayer, nSlotID );	
		if( pAchievementData &&
			pAchievementData->nRewardSkill != INVALID_ID )
		{
			if( nCursorSkillID == INVALID_ID &&
				nSlotID != INVALID_ID)
			{
				x_AchievementPlayerSelect( pPlayer, INVALID_ID, nSlotID ); //bit of a hack right now
			}
			UISetCursorSkill(pAchievementData->nRewardSkill, TRUE);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAchievementsOnMouseHover(
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

	float x = (float)wParam;
	float y = (float)lParam;
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( AppIsTugboat() &&
		 component->m_pParent && 
		 !UIComponentCheckBounds(component->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	x -= pos.m_fX;
	y -= pos.m_fY;

	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(component->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(component->m_bNoScale);
	float fScrollOffsetY = UIComponentGetScrollPos(component).m_fY;
	if( AppIsTugboat() &&
		component->m_pParent )
	{
		x -= component->m_Position.m_fX;
		logx -= component->m_Position.m_fX;
		fScrollOffsetY += UIComponentGetScrollPos(component->m_pParent).m_fY;
	}
	logy += fScrollOffsetY;

	int nSkillID = INVALID_LINK;
	
	UI_GFXELEMENT *pChild = component->m_pGfxElementFirst;
	while(pChild)
	{
		if ( UIElementCheckBounds(pChild, logx, logy ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID &&
			(pChild->m_qwData >> 16 == 0))
		{
			skillpos.m_fX = pChild->m_Position.m_fX;
			skillpos.m_fY = pChild->m_Position.m_fY;

			nSkillID = (int)(pChild->m_qwData & 0xFFFF);

			break;
		}
		pChild = pChild->m_pNextElement;
	}


	skillpos.m_fY -= fScrollOffsetY;
	skillpos.m_fX *= UIGetScreenToLogRatioX(component->m_bNoScale);
	skillpos.m_fY *= UIGetScreenToLogRatioY(component->m_bNoScale);

	
	if (nSkillID != INVALID_LINK)
	{
		const SKILL_DATA* pSkillData = (SKILL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLS, nSkillID);
		if (pSkillData)
		{
			UI_RECT rectIcon;
			rectIcon.m_fX1 = skillpos.m_fX;
			rectIcon.m_fY1 = skillpos.m_fY;

			rectIcon.m_fX2 = rectIcon.m_fX1 + 24;
			rectIcon.m_fY2 = rectIcon.m_fY1 + 24;
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
void InitComponentTypesAchievements(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name						function pointer
		{ "UIAchievementsOnActivate",			UIAchievementsOnActivate },
		{ "UIAchievementsOnPostActivate",		UIAchievementsOnPostActivate },
		{ "UIAchievementsOnPaint",				UIAchievementsOnPaint },
		{ "UIAchievementsOnProgressChange",		UIAchievementsOnProgressChange },
		{ "UIAchievementColumnOnClick",			UIAchievementColumnOnClick },
		{ "UIAchievementIconColumnOnLButtonDown",		UIAchievementIconColumnOnLButtonDown },
		{ "UIAchievementsScrollBarOnScroll",	UIAchievementsScrollBarOnScroll },
		{ "UIAchievementsScrollBarOnMouseWheel",UIAchievementsScrollBarOnMouseWheel },
		{ "UIAchievementsOnMouseHover",			UIAchievementsOnMouseHover },
		{ "UIAchievementSlotOnPaint",			UIAchievementSlotOnPaint },
		{ "UIAchievementSlotsOnPaint",			UIAchievementSlotsOnPaint },
		{ "UIAchievementsOnLButtonDown",		UIAchievementsOnLButtonDown },
		{ "UIAchievementSlotsOnLButtonDown",	UIAchievementSlotsOnLButtonDown },
		{ "UIAchievementSlotOnLButtonDown",		UIAchievementSlotOnLButtonDown },
		
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif


