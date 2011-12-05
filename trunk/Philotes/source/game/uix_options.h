//----------------------------------------------------------------------------
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __UIX_OPTIONS_H_
#define __UIX_OPTIONS_H_

void UIOptionsRemapKey(
	int nKeyCommandBeingRemapped,
	int nKeyAssignBeingRemapped, 
	int nKey,
	int nModifierKey);

//
void UIOptionsDeinit(void);

// Dialog control.
void UIShowOptionsDialog(void);
void UIHideOptionsDialog(void);

// General options dialog message handlers.
UI_MSG_RETVAL UIOptionsDialogOnPostInactivate(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOptionsDialogOnPostActivate(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOptionsDialogAccept(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOptionsDialogCancel(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);

// Graphics options message handlers.
UI_MSG_RETVAL UIOptionsGammaAdjust(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIScreenResolutionComboOnSelect(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIWindowedButtonClicked(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
//UI_MSG_RETVAL UIScreenAspectRatioComboOnPostCreate(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIScreenAspectRatioComboOnSelect(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);

// Sound options message handlers.
UI_MSG_RETVAL UISpeakerConfigOnPostCreate(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISpeakerConfigOnSelect(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);

// General options message handlers.
UI_MSG_RETVAL UIScaleOnPostCreate(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIScaleOnSelect(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam);

// Xfire set connection status
void UIOptionsXfireSetStatus(BOOL bConnected, BOOL bLoginError);

void InitComponentTypesOptions(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

// CHB 2007.09.27 - This is the mechanism used to restore the
// gamma setting in the options.
void UIOptionsNotifyRestoreEvent(void);

#endif
