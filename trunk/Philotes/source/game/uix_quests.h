//----------------------------------------------------------------------------
// uix_quests.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_QUESTS_H_
#define _UIX_QUESTS_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "uix_priv.h"
#include "uix_components_hellgate.h"

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UITYPE_QUEST_PANEL = NUM_UITYPES_HELLGATE,
	NUM_UITYPES_QUEST,
};

//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
struct UI_QUEST_PANEL : UI_COMPONENT
{
	int					m_nSelectedQuest;
	int					m_nNumQuests;
	int					m_nNumSteps;
	int					m_nQuestScrollPos;
	int					m_nStepsScrollPos;
	int					m_nMaxQuestScrollPos;
	int					m_nMaxStepsScrollPos;
	int					m_nQuestStepPortraitUnitClass[MAX_CONVERSATION_SPEAKERS];				
	UNITID				m_idQuestStepPortraitUnit[MAX_CONVERSATION_SPEAKERS];				
	int					m_nMurmurClass;				
	UNITID				m_idMurmur;			
	BOOL				m_bMurmurText;
};

//----------------------------------------------------------------------------
// CASTING FUNCTIONS
//----------------------------------------------------------------------------

CAST_FUNC(UITYPE_QUEST_PANEL, UI_QUEST_PANEL, QuestPanel);

// -- Load functions ----------------------------------------------------------
BOOL UIXmlLoadQuestPanel	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
void InitComponentTypesQuest(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

void UIUpdateQuestPanel(
	void);

void UIResetQuestPanel(
	void);

#endif // _UIX_QUESTS_H_