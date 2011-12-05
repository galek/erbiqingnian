//----------------------------------------------------------------------------
// uix_social.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_SOCIAL_H_
#define _UIX_SOCIAL_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

void InitComponentTypesSocial(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

void UIToggleBuddylist(
	void);

void UIHideBuddylist(
	void);

void UIBuddyListUpdate(
	void);

BOOL UIRemoveCurBuddy(
	void);

void UIToggleGuildPanel(
	void);

void UIHideGuildPanel(
	void);

void UIGuildPanelUpdate(
	void);

void UIHidePartyPanel(
	void);

void UITogglePartyPanel(
	void);

void UIOpenScoreboard(
	void);

void UICloseScoreboard(
	void);

#endif