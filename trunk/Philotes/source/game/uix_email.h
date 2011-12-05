//----------------------------------------------------------------------------
// uix_email.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_EMAIL_H_
#define _UIX_EMAIL_H_


#ifndef _S_PLAYER_EMAIL_H_
#include "s_playerEmail.h"
#endif


//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include "uix_components.h"

void UIEmailInit(
	void);

void UIEmailRepaint(
	void);

void UIToggleEmail(
	void);

void UIEmailEnterReadMode(
	void);

void UIEmailNewMailNotification(
	void);

void UIEmailEnterComposeMode(
	WCHAR *szRecipient = NULL,
	WCHAR *szSubject = NULL,
	BOOL bPopulateFields = TRUE);

void UIEmailUpdateComposeButtonState(
	void);

void UIEmailScrollUp(
	void);

void UIEmailScrollDown(
	void);

void UIEmailUpdateSortLabel(
	void);

void InitComponentTypesEmail(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

void UIEmailOnSendResult(
	PLAYER_EMAIL_RESULT sendResult );

void UIEmailOnUpdate(
	void );

#endif
