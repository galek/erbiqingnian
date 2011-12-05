//----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)

#include "uix.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_scheduler.h"
#include "uix_menus.h"
#include "uix_options.h"
#include "uix_chat.h"
#include "globalindex.h"
#include "appcommontimer.h"
#include "player.h"			// for c_PlayerClearAllMovement()
#include "e_settings.h"
#include "e_gamma.h"		// CHB 2007.10.02
#include "e_screen.h"		// CHB 2007.01.25 - For screen resolution options.
#include "c_input.h"
#include "c_sound_util.h"
#include "c_sound_priv.h"
#include "c_voicechat.h"
#include "gfxoptions.h"		// CHB 2006.12.11
#include "e_optionstate.h"	// CHB 2007.02.28
#include "e_featureline.h"	// CHB 2007.03.05
#include "gameoptions.h"	// CHB 2007.03.06
#include "controloptions.h"	
#include <memory>			// CHB 2007.07.31 - for std::auto_ptr
#include "launcherinfo.h"	// CHB 2007.08.16
#include "e_main.h"			// CHB 2007.08.16 - for e_DX10IsEnabled()
#include "globalindex.h"
#include "language.h"
#include "c_adclient.h"
#include "ServerSuite/BillingProxy/c_BillingClient.h"

#undef min	// some things really shouldn't be macros
#undef max	// some things really shouldn't be macros
#include <limits>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// Handy macro representations of control names.
#define UI_SCREEN_ASPECT_RATIO_COMBO	"screen aspect ratio combo"
#define UI_SCREEN_RESOLUTION_COMBO		"screen resolution combo"
#define UI_SCREEN_REFRESH_COMBO			"screen refresh combo"
#define UI_MULTISAMPLE_COMBO			"antialiasing combo"
#define UI_SHADOWS_COMBO				"shadows combo"
#define UI_SHADER_COMBO					"shaders combo"
#define UI_MODEL_LOD_COMBO				"model detail combo"
#define UI_TEXTURE_LOD_COMBO			"texture detail combo"
#define UI_DISTANCE_CULL_COMBO			"distance cull combo"
#define UI_ENGINE_VERSION_COMBO			"engine version combo"
#define UI_SPEAKER_CONFIG_COMBO			"speaker config combo" // GS - Sorry, I broke your pretty pattern
#define UI_WARDROBE_LIMIT_COMBO			"wardrobe limit combo"

#define UI_FLUID_EFFECTS_BUTTON			"fluid effects btn"		// CHB - Curse you, Guy!  What's wrong with "SPEAKKER" or "SPEAKERZZ"??  :-)
#define UI_FLUID_EFFECTS_LABEL			"fluid effects label"
#define UI_WINDOWED_BUTTON				"windowed btn"
#define UI_GAMMA_PREVIEW				"gamma reference image"	// CHB - The Wave continues.
#define UI_GAMMA_SLIDER					"gamma adjust slider"
#define UI_GAMMA_PANEL					"video settings gamma sliding panel"
#define UI_DISABLE_AD_CLIENT_BUTTON		"disable ad updates btn" // GS - Yes!  Keepin' the trend alive!
#define UI_DISABLE_AD_CLIENT_LABEL		"disable ad updates label"
#define UI_CHAT_AUTO_FADE_BUTTON		"chat auto fade btn"
#define UI_CHAT_AUTO_FADE_LABEL			"chat auto fade"

#define UI_GAME_ACCOUNT_PANEL			"game account panel"
#define UI_ACCOUNT_TIME_REMAINING_LABEL "options account time remaining label"
#define UI_ACCOUNT_STATUS_LABEL			"account status displayed"

// This is the table of combo boxes on the graphics options dialog.
// The list is used to do things like:
//	*	Initialize the combos
//	*	Implement mutual exclusivity where other combos will close
//		when a new one is opened
static const char * const s_aComboList[] =
{
	UI_SCREEN_ASPECT_RATIO_COMBO,
	UI_SCREEN_RESOLUTION_COMBO,
	UI_SCREEN_REFRESH_COMBO,
	UI_MULTISAMPLE_COMBO,
	UI_SHADOWS_COMBO,
	UI_SHADER_COMBO,
	UI_MODEL_LOD_COMBO,
	UI_TEXTURE_LOD_COMBO,
	UI_DISTANCE_CULL_COMBO,
	UI_ENGINE_VERSION_COMBO,
	UI_SPEAKER_CONFIG_COMBO,
	UI_WARDROBE_LIMIT_COMBO,
};

// This is a table mapping feature lines to controls.
struct FeatureComboInfo
{
	const char * pComboName;
	DWORD nLineName;
};
const FeatureComboInfo s_aFeatureCombos[] =
{
	{	UI_MULTISAMPLE_COMBO,		E_FEATURELINE_FOURCC("MSAA")	},
	{	UI_SHADOWS_COMBO,			E_FEATURELINE_FOURCC("SHDW")	},
	{	UI_SHADER_COMBO,			E_FEATURELINE_FOURCC("SHDR")	},
	{	UI_MODEL_LOD_COMBO,			E_FEATURELINE_FOURCC("MLOD")	},
	{	UI_TEXTURE_LOD_COMBO,		E_FEATURELINE_FOURCC("TLOD")	},
	{	UI_DISTANCE_CULL_COMBO,		E_FEATURELINE_FOURCC("DIST")	},
	{	UI_WARDROBE_LIMIT_COMBO,	E_FEATURELINE_FOURCC("WLIM")	},
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
struct UI_OPTIONS_PARAMS
{
	SOUND_CONFIG_OPTIONS	tSoundOptions;
	MUSIC_CONFIG_OPTIONS	tMusicOptions;
	SOUND_CONFIG_OPTIONS	tSoundOptionsOriginal;
	MUSIC_CONFIG_OPTIONS	tMusicOptionsOriginal;
	GAMEOPTIONS				tGameOptions;	// CHB 2007.03.06
	CONTROLOPTIONS			tControlOptions;	
} sgtOptions;


//----------------------------------------------------------------------------
//
// GAPHICS OPTIONS
//
//----------------------------------------------------------------------------

static CComPtr<OptionState> s_pOriginalState;
static CComPtr<OptionState> s_pWorkingState;
static CComPtr<FeatureLineMap> s_pOriginalFeatureMap;
static CComPtr<FeatureLineMap> s_pWorkingFeatureMap;
static E_SCREEN_DISPLAY_MODE_GROUP * s_pvScreenResolutionList = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void UIDeleteScreenResolutionList(void)
{
	delete s_pvScreenResolutionList;
	s_pvScreenResolutionList = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sCleanupState(void)
{
	s_pOriginalState = 0;
	s_pWorkingState = 0;
	s_pOriginalFeatureMap = 0;
	s_pWorkingFeatureMap = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIOptionsDeinit(void)
{
	sCleanupState();
	UIDeleteScreenResolutionList();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sInitState(void)
{
	sCleanupState();
	V_DO_FAILED(e_OptionState_CloneActive(&s_pOriginalState))
	{
		return false;
	}
	V_DO_FAILED(e_OptionState_CloneActive(&s_pWorkingState))
	{
		return false;
	}
	V_DO_FAILED(e_FeatureLine_OpenMap(s_pOriginalState, &s_pOriginalFeatureMap))
	{
		return false;
	}
	V_DO_FAILED(e_FeatureLine_OpenMap(s_pWorkingState, &s_pWorkingFeatureMap))
	{
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
PRESULT sCommitWorkingState(void)
{
	V_RETURN(e_FeatureLine_CommitToActive(s_pWorkingFeatureMap));
	V_RETURN(e_OptionState_CommitToActive(s_pWorkingState));
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
PRESULT sCommitOriginalState(void)
{
	V_RETURN(e_FeatureLine_CommitToActive(s_pOriginalFeatureMap));
	V_RETURN(e_OptionState_CommitToActive(s_pOriginalState));
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.08.16
static
bool sGetDX10(void)
{
	LauncherInfo li;
	li.bNoSave = true;
	if (li.success)
	{
		return !!li.nDX10;
	}
	return !!e_DX10IsEnabled();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.08.16
static
UI_COMBOBOX * sGetEngineVersionCombo(void)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETNULL(pDialog != 0);
	UI_COMPONENT * pComboComp = UIComponentFindChildByName(pDialog, UI_ENGINE_VERSION_COMBO);
	if (pComboComp == 0)
	{
		return 0;
	}
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	ASSERT_RETNULL(pCombo != 0);
	return pCombo;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.08.16
static
void sPopulateEngineVersionCombo(void)
{
	UI_COMBOBOX * pCombo = sGetEngineVersionCombo();
	if (pCombo == 0)
	{
		return;
	}

	int nDX9Index = -1;
	int nDX10Index = -1;
	int nCurIndex = 0;

	UITextBoxClear(pCombo->m_pListBox);

#if ! ISVERSION(DEMO_VERSION)
	if (SupportsDX10())
	{
		UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_DIRECTX10), 1);
		nDX10Index = nCurIndex++;
	}
#endif

	if (SupportsDX9())
	{
		UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet(GS_DIRECTX9), 0);
		nDX9Index = nCurIndex++;
	}

	// CHB 2007.09.12
	UIComponentSetActive(pCombo, nCurIndex > 1);

	int nIndex = nDX9Index;
	if ((nDX10Index >= 0) && sGetDX10())
	{
		nIndex = nDX10Index;
	}
	UIComboBoxSetSelectedIndex(pCombo, nIndex, false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sGetDX10Selected(void)
{
	UI_COMBOBOX * pCombo = sGetEngineVersionCombo();
	if (pCombo == 0)
	{
		return false;
	}
	if (UIComboBoxGetSelectedIndex(pCombo) < 0)
	{
		return false;
	}
	return !!UIComboBoxGetSelectedData(pCombo);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sHasEngineVersionChanged(void)
{
	bool bSelectedDX10 = sGetDX10Selected();
	bool bCurrentDX10 = !!e_DX10IsEnabled();
	return bCurrentDX10 != bSelectedDX10;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
UI_COMBOBOX * sGetSpeakerConfigCombo(void)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETNULL(pDialog != 0);
	UI_COMPONENT * pComboComp = UIComponentFindChildByName(pDialog, UI_SPEAKER_CONFIG_COMBO);
	if (pComboComp == 0)
	{
		return 0;
	}
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	ASSERT_RETNULL(pCombo != 0);
	return pCombo;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
int sGetSelectedSpeakerConfig(void)
{
	UI_COMBOBOX * pCombo = sGetSpeakerConfigCombo();
	if (pCombo == 0)
	{
		return 0;
	}
	if (UIComboBoxGetSelectedIndex(pCombo) < 0)
	{
		return 0;
	}
	return (int)UIComboBoxGetSelectedData(pCombo);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sHasSpeakerConfigChanged(void)
{
	if(sGetSelectedSpeakerConfig() != c_SoundGetSpeakerConfig())
		return true;
	return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
UI_BUTTONEX * sGetAdUpdateDisabledButton(void)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETNULL(pDialog != 0);
	UI_COMPONENT * pButtonComp = UIComponentFindChildByName(pDialog, UI_DISABLE_AD_CLIENT_BUTTON);
	if (pButtonComp == 0)
	{
		return 0;
	}
	UI_BUTTONEX * pButton = UICastToButton(pButtonComp);
	ASSERT_RETNULL(pButton != 0);
	return pButton;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sGetSelectedAdConfig(void)
{
	UI_BUTTONEX * pButton = sGetAdUpdateDisabledButton();
	if (pButton == 0)
	{
		return FALSE;
	}
	return !!UIButtonGetDown(pButton);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sHasAdConfigChanged(void)
{
	if( AppIsTugboat())
	{
		return false;
	}
	if(AppIsSinglePlayer() && !AppIsDemo() && !AppIsBeta())
	{
		if(sGetSelectedAdConfig() != c_AdClientIsDisabled())
			return true;
		return false;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sAcceptEngineVersion(void)
{
	LauncherSetDX10(sGetDX10Selected());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const unsigned MAX_CHOICE_INTERNAL_NAME_LENGTH = 32;
const unsigned MAX_CHOICE_DISPLAY_NAME_LENGTH = 256;
static
void sRefreshCombo(FeatureLineMap * pMap, UI_COMPONENT * pComponent, DWORD nLineName)
{
	// Get combo.
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETURN(pCombo != 0);

	// First, get existing setting.

	// Now empty the combo.
	UITextBoxClear(pCombo->m_pListBox);

	// Then fill it with the right values.
	for (unsigned i = 0; /**/; ++i)
	{
		wchar_t buf[MAX_CHOICE_DISPLAY_NAME_LENGTH];
		unsigned nSize = _countof(buf);
		PRESULT nResult = pMap->EnumerateChoiceDisplayNames(nLineName, i, buf, nSize);
		if (nResult == E_NO_MORE_ITEMS)
		{
			break;
		}
		V_BREAK(nResult);
		UIListBoxAddString(pCombo->m_pListBox, buf);
	}

	// Select the previously chosen option if available.
	unsigned nIndex;
	PRESULT nResult = pMap->GetSelectedChoice(nLineName, nIndex);
	if (FAILED(nResult))
	{
		nIndex = 0;
	}
	UIComboBoxSetSelectedIndex(pCombo, nIndex, FALSE);
}

static
void sRefreshCombo(FeatureLineMap * pMap, const char * pComponentName, DWORD nLineName)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETURN(pDialog != 0);
	UI_COMPONENT * pComboComp = UIComponentFindChildByName(pDialog, pComponentName);
	if (pComboComp != 0)
		sRefreshCombo(pMap, pComboComp, nLineName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIDoWhatIWant(UI_COMPONENT * pComponent, bool bVisible)
{
	ASSERT_RETURN(pComponent != 0);
	if (bVisible)
	{
		UIComponentSetActive(pComponent, true);
	}
	else
	{
		UIComponentSetVisible(pComponent, false);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIRefreshDialog(void)
{
	ASSERT_RETURN( s_pWorkingFeatureMap );
	V_DO_FAILED(s_pWorkingFeatureMap->ReMap())
	{
		return;
	}
	for (unsigned i = 0; i < _countof(s_aFeatureCombos); ++i)
	{
		sRefreshCombo(s_pWorkingFeatureMap, s_aFeatureCombos[i].pComboName, s_aFeatureCombos[i].nLineName);
	}

	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETURN(pDialog != 0);
	UI_COMPONENT * pComp = UIComponentFindChildByName(pDialog, "restart needed label");
	if (pComp != 0)
	{
		bool bDeferred = s_pWorkingState->HasDeferrals() || sHasEngineVersionChanged() || sHasSpeakerConfigChanged() || sHasAdConfigChanged();
		sUIDoWhatIWant(pComp, bDeferred);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIComboClose(UI_COMPONENT * pComponent)
{
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	if (UIButtonGetDown(pCombo->m_pButton))
	{
//		UIWindowshadeButtonOnClick(pCombo->m_pButton, UIMSG_LBUTTONCLICK, 0, 0);
//		UIComponentHandleUIMessage(pCombo->m_pButton, UIMSG_LBUTTONCLICK, 0, 0);

		// Of the four ways I tried, this works.
		UIButtonSetDown(pCombo->m_pButton, false);
		UIComponentHandleUIMessage(pCombo->m_pButton, UIMSG_PAINT, 0, 0);
		UIComponentWindowShadeClosed(pCombo->m_pButton->m_pNextSibling);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.09.27
static
void sSetGammaFromSlider(void)
{
	if (s_pWorkingState == 0)
	{
		return;
	}

	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETURN(pDialog != 0);

	UI_COMPONENT * pComponent = UIComponentFindChildByName(pDialog, UI_GAMMA_SLIDER);
	ASSERT_RETURN(pComponent != 0);

	if (!UIComponentGetActive(pComponent))
	{
		return;
	}

	UI_SCROLLBAR * pScrollbar = UICastToScrollBar(pComponent);
	ASSERT_RETURN(pScrollbar != 0);

	float fGammaSlider = UIScrollBarGetValue(pScrollbar, TRUE);
	float fGammaPower = e_MapGammaPctToPower( fGammaSlider );
	s_pWorkingState->set_fGammaPower(fGammaPower);
	V( e_SetGamma( s_pWorkingState->get_fGammaPower() ) );

	trace( "Event! fGammaSlider = %f, fGammaPower = %f\n", fGammaSlider, fGammaPower );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsGammaAdjust(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	if (!UIComponentGetActive(component))	
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	sSetGammaFromSlider();
	if ( msg == UIMSG_SCROLL )
		return UIScrollBarOnScroll( component, msg, wParam, lParam );

	// want to keep going
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.09.27
void UIOptionsNotifyRestoreEvent(void)
{
	sSetGammaFromSlider();

	// Also need to paint reference image.
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	if (pDialog != 0)
	{
		UI_COMPONENT * pComponent = UIComponentFindChildByName(pDialog, UI_GAMMA_PREVIEW);
		if (pComponent != 0)
		{
			UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGammaReferencePaint( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);

	UI_RECT tOuterRect = UIComponentGetRectBase( component );
	BYTE byDarker = ((BYTE*)&component->m_dwColor)[2];
	DWORD dwDarker = ARGB_MAKE_FROM_INT( byDarker, byDarker, byDarker, 255 );
	UIComponentAddPrimitiveElement( component, UIPRIM_BOX, 0, tOuterRect, NULL, NULL, dwDarker );

	float fInnerScale = 0.55f;
	float fOx = tOuterRect.Width()  * fInnerScale / 2;
	float fOy = tOuterRect.Height() * fInnerScale / 2;
	UI_RECT tInnerRect( fOx, fOy, tOuterRect.m_fX2 - fOx, tOuterRect.m_fY2 - fOy );
	BYTE byLighter = ((BYTE*)&component->m_dwColor)[1];
	DWORD dwLighter = ARGB_MAKE_FROM_INT( byLighter, byLighter, byLighter, 255 );
	UIComponentAddPrimitiveElement( component, UIPRIM_BOX, 0, tInnerRect, NULL, NULL, dwLighter );

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsButtonCheckNeedRestart(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_BUTTONEX* button = UICastToButton(component);
	if (wParam == 0 && !UIComponentCheckBounds(button))
	{
		//button->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_MSG_RETVAL eRetVal = UIButtonOnButtonDown(component, msg, wParam, lParam);

	sUIRefreshDialog();

	return eRetVal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sKeyDefaultsCallbackOk(
	void *, 
	DWORD )
{
	sgtOptions.tControlOptions.SetToDefault();

	UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	if (pDialog)
	{
		UIButtonSetDown(pDialog, "shift as modifier btn", sgtOptions.tControlOptions.bShiftAsModifier);
		UIButtonSetDown(pDialog, "ctrl as modifier btn", sgtOptions.tControlOptions.bCtrlAsModifier);
		UIButtonSetDown(pDialog, "alt as modifier btn", sgtOptions.tControlOptions.bAltAsModifier);
	}

	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), UIMSG_PAINT, 0, 0);
}


UI_MSG_RETVAL UIOptionsControlsDefault(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	InputEndKeyRemap();
	DIALOG_CALLBACK tOkCallback;
	DialogCallbackInit( tOkCallback );
	tOkCallback.pfnCallback = sKeyDefaultsCallbackOk;

	UIShowGenericDialog(GlobalStringGet(GS_CONFIRM_DIALOG_HEADER), GlobalStringGet(GS_CONFIRM_DEFAULT_KEY_SETTINGS), TRUE, &tOkCallback, 0, AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef WORD ASPECT_RATIO_TYPE;
#define ASPECT_RATIO(n, d) static_cast<ASPECT_RATIO_TYPE>(((n) << 8) | (d))
#define ASPECT_RATIO_ALL ASPECT_RATIO(0, 0)
#define ASPECT_RATIO_NUMERATOR(x) (((x) >> 8) & 0xff)
#define ASPECT_RATIO_DENOMINATOR(x) ((x) & 0xff)
const ASPECT_RATIO_TYPE AspectRatioTable[] = { ASPECT_RATIO(16, 10), ASPECT_RATIO(16, 9), ASPECT_RATIO(4, 3), ASPECT_RATIO(5, 4), ASPECT_RATIO_ALL };
static unsigned snAspectRatioPresentMask = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sUIIsAspectRatio(const E_SCREEN_DISPLAY_MODE & Mode, ASPECT_RATIO_TYPE nAR)
{
	return Mode.Width * ASPECT_RATIO_DENOMINATOR(nAR) == Mode.Height * ASPECT_RATIO_NUMERATOR(nAR);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
int sUIGetAspectRatioTableIndex(const E_SCREEN_DISPLAY_MODE & Mode)
{
	int nIndexAll = -1;
	for (int i = 0; i < _countof(AspectRatioTable); ++i)
	{
		if (sUIIsAspectRatio(Mode, AspectRatioTable[i]))
		{
			return i;
		}
		if (AspectRatioTable[i] == ASPECT_RATIO_ALL)
		{
			nIndexAll = i;
		}
	}
	ASSERT_RETVAL(false, nIndexAll);
}

static
const ASPECT_RATIO_TYPE sUIGetAspectRatio(const E_SCREEN_DISPLAY_MODE & Mode)
{
	int i = sUIGetAspectRatioTableIndex(Mode);
	return ((0 <= i) && (i < _countof(AspectRatioTable))) ? AspectRatioTable[i] : ASPECT_RATIO_ALL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
const ASPECT_RATIO_TYPE sUIGetAspectRatioFromComboIndex(int nComboIndex)
{
	int nIndexAll = 0;
	for (int i = 0; i < _countof(AspectRatioTable); ++i)
	{
		if ((snAspectRatioPresentMask & (1 << i)) != 0)
		{
			if (nComboIndex == 0)
			{
				return AspectRatioTable[i];
			}
			--nComboIndex;
		}
		if (AspectRatioTable[i] == ASPECT_RATIO_ALL)
		{
			nIndexAll = i;
		}
	}
	return AspectRatioTable[nIndexAll];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
const ASPECT_RATIO_TYPE sUIGetSelectedAspectRatio(UI_COMPONENT * pDialog)
{
	int nComboIndex = UIComboBoxGetSelectedIndex(pDialog, UI_SCREEN_ASPECT_RATIO_COMBO);
	return sUIGetAspectRatioFromComboIndex(nComboIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// A two-step process is needed because changing res can cause
// the UI to reload, which will wipe out the aspect ratio
// combo and the information of its current selection.
static ASPECT_RATIO_TYPE s_nSelectedAspectRatio = ASPECT_RATIO_ALL;
static ASPECT_RATIO_TYPE s_nLatchedAspectRatio = ASPECT_RATIO_ALL;

static
void sSaveAspectRatioSelection(void)
{
	s_nSelectedAspectRatio = s_nLatchedAspectRatio;
}

static
void sLatchAspectRatioSelection(void)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	if (pDialog != 0)
	{
		s_nLatchedAspectRatio = sUIGetSelectedAspectRatio(pDialog);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIUpdateGraphicsOptionsUIState(void)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETURN(pDialog != 0);

	bool bWindowed = !!UIButtonGetDown(pDialog, UI_WINDOWED_BUTTON);

	// Show/hide refresh rate depending on windowed selection.
	UI_COMPONENT * pRefreshCombo = UIComponentFindChildByName(pDialog, UI_SCREEN_REFRESH_COMBO);
	ASSERT_RETURN(pRefreshCombo != 0);
	UI_COMPONENT * pRefreshLabel = UIComponentFindChildByName(pDialog, "screen refresh label");
	ASSERT_RETURN(pRefreshLabel != 0);

	sUIDoWhatIWant(pRefreshCombo, !bWindowed);
	sUIDoWhatIWant(pRefreshLabel, !bWindowed);

	// CHB 2007.02.02 - Close the refresh rate combo when it is hidden.
	if (bWindowed)
	{
		sUIComboClose(pRefreshCombo);
	}

	// CHB 2007.10.02
	{
		// Use of the *active* state here is significant. We don't
		// want the slider to appear unless sliding it will have
		// instantly visible feedback.
		bool bGammaAvailable = e_CanDoGamma(e_OptionState_GetActive(), e_OptionState_GetActive()->get_bWindowed());
		UI_COMPONENT * pPanel = UIComponentFindChildByName(pDialog, UI_GAMMA_PANEL);
		ASSERT_RETURN(pPanel != 0);
		sUIDoWhatIWant(pPanel, bGammaAvailable);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
const E_SCREEN_DISPLAY_MODE_GROUP & UIGetScreenResolutionList(void)
{
	ASSERT(s_pvScreenResolutionList != 0);
	return *s_pvScreenResolutionList;
}

static
void UISnapshotScreenResolutionList(void)
{
	UIDeleteScreenResolutionList();
	s_pvScreenResolutionList = new E_SCREEN_DISPLAY_MODE_GROUP(e_Screen_EnumerateDisplayModesGroupedByRefreshRate());

	DWORD nMask = 0;
	for (int i = 0; i < _countof(AspectRatioTable); ++i)
	{
		const ASPECT_RATIO_TYPE nAR = AspectRatioTable[i];
		bool bFound = false;
		if (nAR == ASPECT_RATIO_ALL)
		{
			bFound = true;
		}
		else
		{
			for (E_SCREEN_DISPLAY_MODE_GROUP::const_iterator j = UIGetScreenResolutionList().begin(); j != UIGetScreenResolutionList().end(); ++j)
			{
				if (sUIIsAspectRatio(j->front(), nAR))
				{
					bFound = true;
					break;
				}
			}
		}
		nMask |= bFound << i;
	}
	snAspectRatioPresentMask = nMask;
}

static
const E_SCREEN_DISPLAY_MODE sUIGetCurrentScreenResolution(void)
{
	E_SCREEN_DISPLAY_MODE CurMode;
	e_Screen_DisplayModeFromState(s_pWorkingState, CurMode);
	CurMode.Format = e_Screen_GetCurrentDisplayMode().Format;
	return CurMode;
}

static
int sUIFindScreenResolutionIndex(const E_SCREEN_DISPLAY_MODE & Mode)
{
	const E_SCREEN_DISPLAY_MODE_GROUP & g = UIGetScreenResolutionList();
	int nSize = static_cast<int>(g.size());
	for (int i = 0; i < nSize; ++i)
	{
		if (e_Screen_AreDisplayModesEqualAsideFromRefreshRate(Mode, g[i].front()))
		{
			return i;
		}
	}
	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIScreenAspectRatioComboSetSelection(UI_COMPONENT * pComboComp, ASPECT_RATIO_TYPE nARSelected)
{
	ASSERT_RETURN(pComboComp);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	UITextBoxClear(pCombo->m_pListBox);

	int nSelectionIndex = -1;
	int nIndexAll = -1;
	int nComboIndex = 0;
	for (int i = 0; i < _countof(AspectRatioTable); ++i)
	{
		const ASPECT_RATIO_TYPE nAR = AspectRatioTable[i];
		if (nAR == ASPECT_RATIO_ALL)
		{
			UIListBoxAddString(pCombo->m_pListBox, GlobalStringGet( GS_DROPDOWN_ALL ));
			nIndexAll = nComboIndex++;
		}
		else if ((snAspectRatioPresentMask & (1 << i)) != 0)
		{
			WCHAR Str[16];
			PStrPrintf(Str, _countof(Str), L"%u:%u", ASPECT_RATIO_NUMERATOR(nAR), ASPECT_RATIO_DENOMINATOR(nAR));
			UIListBoxAddString(pCombo->m_pListBox, Str);
			if (nAR == nARSelected)
			{
				nSelectionIndex = nComboIndex;
			}
			++nComboIndex;
		}
	}

	nSelectionIndex = (nSelectionIndex < 0) ? nIndexAll : nSelectionIndex;
	UIComboBoxSetSelectedIndex(pCombo, nSelectionIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIScreenResolutionComboSetSelection(UI_COMPONENT * pComboComp, const E_SCREEN_DISPLAY_MODE & Mode, ASPECT_RATIO_TYPE nARFilter)
{
	ASSERT_RETURN(pComboComp);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	UITextBoxClear(pCombo->m_pListBox);

	int nSelectedIndex = -1;
	int nBestDelta = INT_MAX;
	int nCurrentIndex = 0;

	const E_SCREEN_DISPLAY_MODE_GROUP & v = UIGetScreenResolutionList();
	for (E_SCREEN_DISPLAY_MODE_GROUP::const_iterator i = v.begin(); i != v.end(); ++i)
	{
		int nW = i->front().Width;
		int nH = i->front().Height;
		if (sUIIsAspectRatio(i->front(), nARFilter))
		{
			WCHAR Str[32];
			PStrPrintf(Str, _countof(Str), L"%4u x %-4u", nW, nH);
			UIListBoxAddString(pCombo->m_pListBox, Str);

			int nDelta = nW * nH - static_cast<int>(Mode.Width * Mode.Height);
			nDelta = (nDelta < 0) ? -nDelta : nDelta;	// absolute value
			if (nDelta < nBestDelta)
			{
				nSelectedIndex = nCurrentIndex;
				nBestDelta = nDelta;
			}

			++nCurrentIndex;
		}
	}
//	int nIndex = sUIFindScreenResolutionIndex(Mode);
	UIComboBoxSetSelectedIndex(pCombo, nSelectedIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
const E_SCREEN_DISPLAY_MODE sUIGetSelectedScreenMode(UI_COMPONENT * pDialog, int nIndex, ASPECT_RATIO_TYPE nAR)
{
	E_SCREEN_DISPLAY_MODE Mode = sUIGetCurrentScreenResolution();

	const E_SCREEN_DISPLAY_MODE_GROUP & screenreslist = UIGetScreenResolutionList();
	ASSERT_RETVAL(&screenreslist != 0, Mode);

	int nCurrentIndex = 0;
	for (E_SCREEN_DISPLAY_MODE_GROUP::const_iterator i = UIGetScreenResolutionList().begin(); i != UIGetScreenResolutionList().end(); ++i)
	{
		if (sUIIsAspectRatio(i->front(), nAR))
		{
			if (nCurrentIndex == nIndex)
			{
				const E_SCREEN_DISPLAY_MODE_VECTOR & v = *i;
				Mode = v.back();
				int nRefresh = UIComboBoxGetSelectedIndex(pDialog, UI_SCREEN_REFRESH_COMBO);
				if ((0 <= nRefresh) && (nRefresh < static_cast<int>(v.size())))
				{
					Mode = v[nRefresh];
				}
				break;
			}
			++nCurrentIndex;
		}
	}

	return Mode;
}

static
const E_SCREEN_DISPLAY_MODE sUIGetSelectedScreenMode_WithRefreshIndex(UI_COMPONENT * pDialog, int nIndex)
{
	return sUIGetSelectedScreenMode(pDialog, nIndex, sUIGetSelectedAspectRatio(pDialog));
}

static
const E_SCREEN_DISPLAY_MODE sUIGetSelectedScreenMode_WithAspectRatio(UI_COMPONENT * pDialog, ASPECT_RATIO_TYPE nAR)
{
	return sUIGetSelectedScreenMode(pDialog, UIComboBoxGetSelectedIndex(pDialog, UI_SCREEN_RESOLUTION_COMBO), nAR);
}

static
const E_SCREEN_DISPLAY_MODE sUIGetSelectedScreenMode(UI_COMPONENT * pDialog)
{
	return sUIGetSelectedScreenMode_WithRefreshIndex(pDialog, UIComboBoxGetSelectedIndex(pDialog, UI_SCREEN_RESOLUTION_COMBO));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIScreenRefreshComboSetSelection(UI_COMPONENT * pComboComp, const E_SCREEN_DISPLAY_MODE & Mode)
{
	ASSERT_RETURN(pComboComp);
	
//	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
//	ASSERT_RETURN(pDialog);

	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	UITextBoxClear(pCombo->m_pListBox);

	int nRefresh = 0;
	float fBestDelta = std::numeric_limits<float>::max();

	int nIndex = sUIFindScreenResolutionIndex(Mode);
	if (nIndex >= 0)
	{
		E_SCREEN_DISPLAY_MODE TargetMode = Mode;
		if (TargetMode.IsWindowed())
		{
			TargetMode.DefaultFullscreen();
		}

		const E_SCREEN_DISPLAY_MODE_VECTOR & v = UIGetScreenResolutionList()[nIndex];
		for (E_SCREEN_DISPLAY_MODE_VECTOR::const_iterator i = v.begin(); i != v.end(); ++i)
		{
			const int MAX_STRING = 32;
			WCHAR Str[ MAX_STRING ];
			const WCHAR *puszFormat = GlobalStringGet( GS_DROPDOWN_HERTZ );
			PStrPrintf(Str, MAX_STRING, puszFormat, i->RefreshRate.AsInt());
			UIListBoxAddString(pCombo->m_pListBox, Str);

			float t = i->RefreshRate.AsFloat() - TargetMode.RefreshRate.AsFloat();
			t = (t < 0) ? -t : t;
			if (t < fBestDelta)
			{
				fBestDelta = t;
				nRefresh = static_cast<int>(i - v.begin());
			}
		}
	}
	UIComboBoxSetSelectedIndex(pCombo, nRefresh);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScreenAspectRatioComboOnSelect(UI_COMPONENT * pComponent, int /*msg*/, DWORD wParam, DWORD /*lParam*/)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT * pResCombo = UIComponentFindChildByName(pDialog, UI_SCREEN_RESOLUTION_COMBO);
	if (pResCombo != 0)
	{
		ASPECT_RATIO_TYPE nPrevAR = sUIGetAspectRatioFromComboIndex(wParam);
		const E_SCREEN_DISPLAY_MODE PrevMode = sUIGetSelectedScreenMode_WithAspectRatio(pDialog, nPrevAR);
		const ASPECT_RATIO_TYPE nCurrAR = sUIGetSelectedAspectRatio(pDialog);
		sUIScreenResolutionComboSetSelection(pResCombo, PrevMode, nCurrAR);		// this may change the resolution
		E_SCREEN_DISPLAY_MODE NewMode = sUIGetSelectedScreenMode(pDialog);
		NewMode.DefaultFullscreen();	// CHB 2007.06.29 // NewMode.RefreshRate = PrevMode.RefreshRate;

		UI_COMPONENT * pRefCombo = UIComponentFindChildByName(pDialog, UI_SCREEN_REFRESH_COMBO);
		if (pRefCombo != 0)
		{
			sUIScreenRefreshComboSetSelection(pRefCombo, NewMode);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWindowedButtonClicked(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	// CHB 2007.02.02
	if (!UIComponentGetActive(component))	
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// wParam is 1 if this is a message resulting from a click on the sibling label
	if (wParam != 1 && !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// Hmm, appears we need to call this manually.
	UI_MSG_RETVAL nResult = UIButtonOnButtonDown(component, msg, wParam, lParam);

	UI_BUTTONEX * pButton = UICastToButton(component);
#if 1	// CHB !!! - Temp workaround.
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	E_SCREEN_DISPLAY_MODE Mode = sUIGetSelectedScreenMode(pDialog);
	bool bWindowed = !!UIButtonGetDown(pButton);
	if (bWindowed)
	{
		Mode.SetWindowed();
	}
	e_Screen_DisplayModeToState(s_pWorkingState, Mode);
//	s_pWorkingState->SuspendUpdate();
//	s_pWorkingState->set_nScreenRefreshHz(Mode.RefreshRate);
//	s_pWorkingState->set_bWindowed(!!UIButtonGetDown(pButton));
//	s_pWorkingState->ResumeUpdate();
#else
	s_pWorkingState->set_bWindowed(!!UIButtonGetDown(pButton));
#endif

	sUIUpdateGraphicsOptionsUIState();
	return nResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEnableComponentWithSiblingLabel(UI_COMPONENT * pComponent, bool bEnable)
{
	ASSERT_RETURN(pComponent != 0);
	UIComponentSetActive(pComponent, bEnable);
	UI_COMPONENT * pLabel = pComponent->m_pPrevSibling;
	if ((pLabel != 0) && UIComponentIsLabel(pLabel))
	{
		pLabel->m_dwColor = bEnable ? GFXCOLOR_WHITE : GFXCOLOR_DKGRAY;
		UIComponentHandleUIMessage(pLabel, UIMSG_PAINT, 0, 0);
		UIComponentSetActive(pLabel, bEnable);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScreenResolutionComboOnSelect(
	UI_COMPONENT* component,
	int /*msg*/,
	DWORD wParam,
	DWORD /*lParam*/ )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);

	{
		const E_SCREEN_DISPLAY_MODE Mode = sUIGetSelectedScreenMode(pDialog);
		s_pWorkingState->SuspendUpdate();
		s_pWorkingState->set_nFrameBufferWidth(Mode.Width);
		s_pWorkingState->set_nFrameBufferHeight(Mode.Height);
		s_pWorkingState->ResumeUpdate();
	}

	UI_COMPONENT * pRefCombo = UIComponentFindChildByName(pDialog, UI_SCREEN_REFRESH_COMBO);
	if (pRefCombo != 0)
	{
		//const E_SCREEN_DISPLAY_MODE RefMode = sUIGetSelectedScreenMode_WithRefreshIndex(pDialog, wParam);
		E_SCREEN_DISPLAY_MODE Mode = sUIGetSelectedScreenMode(pDialog);
		Mode.DefaultFullscreen();	// CHB 2007.06.29 // Mode.RefreshRate = RefMode.RefreshRate;
		sUIScreenRefreshComboSetSelection(pRefCombo, Mode);
	}

	// CHB 2007.07.26 - If the selected mode is too big for the
	// current desktop, then force full screen mode.
	{
		UI_COMPONENT * pComponent = UIComponentFindChildByName(pDialog, UI_WINDOWED_BUTTON);
		UI_BUTTONEX * pButton = (pComponent != 0) ? UICastToButton(pComponent) : 0;
		if (pButton != 0)
		{
			const E_SCREEN_DISPLAY_MODE Mode = sUIGetSelectedScreenMode(pDialog);
			if (e_Screen_IsModeValidForWindowedDisplay(Mode))
			{
				UIEnableComponentWithSiblingLabel(pButton, true);
			}
			else
			{
				if (UIButtonGetDown(pButton))
				{
					UIComponentHandleUIMessage(pButton, UIMSG_LBUTTONDOWN, 1, 0);
				}
				UIEnableComponentWithSiblingLabel(pButton, false);
			}
		}	
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
void sUIShowScreenResolutionChangeFailedDialogOkCallback(
	void * pContext, 
	DWORD dwCallbackData)
{
	// Go back to options.
	UIShowOptionsDialog();
}

void UIShowScreenResolutionChangeFailedDialog(void)
{
	DIALOG_CALLBACK OkCallback;
	OkCallback.pfnCallback = &sUIShowScreenResolutionChangeFailedDialogOkCallback;
	OkCallback.pCallbackData = 0;
	UIShowGenericDialog(GlobalStringGet(GS_UI_SCREEN_RESOLUTION_CHANGE_FAILED_TITLE), GlobalStringGet(GS_UI_SCREEN_RESOLUTION_CHANGE_FAILED_TEXT), FALSE, &OkCallback, 0, AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO
{
	TIME nEndTime;
	int nLastSeconds;
	DWORD nScheduledEvent;

	UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO(void);
};

UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO::UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO(void)
{
	nLastSeconds = -1;
	nScheduledEvent = INVALID_ID;
}

static
void sUIScreenResolutionConfirmDialogOkCallback(
	void * pContext, 
	DWORD dwCallbackData)
{
	UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO * pInfo = static_cast<UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO *>(pContext);

	// Cancel event.
	if (pInfo->nScheduledEvent != INVALID_ID)
	{
		CSchedulerCancelEvent(pInfo->nScheduledEvent);
		pInfo->nScheduledEvent = INVALID_ID;
	}

	// Save the options now that the user's okayed them.
	GfxOptions_Save();
	sSaveAspectRatioSelection();

	// Clean up.
	delete pInfo;
	sCleanupState();

	// Move on.
	UIRestoreLastMenu();
}

static
void sUIScreenResolutionConfirmDialogCancelCallback(
	void * pContext, 
	DWORD dwCallbackData)
{
	UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO * pInfo = static_cast<UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO *>(pContext);

	// Cancel event.
	if (pInfo->nScheduledEvent != INVALID_ID)
	{
		CSchedulerCancelEvent(pInfo->nScheduledEvent);
		pInfo->nScheduledEvent = INVALID_ID;
	}

	// Restore original screen mode if not confirmed.
	if (SUCCEEDED(sCommitOriginalState()))
	{
		// Original mode restored, go back to options.
		sCleanupState();

		// CHB 2007.06.28 - Bug 17518: Restoring the original screen
		// mode causes the UI to reload, which resets the activation
		// and visible states of the options dialog.
		//UIShowOptionsDialog();
		UISetReloadCallback(&UIShowOptionsDialog);
	}
	else
	{
		// Restoring the original mode failed.
		UIShowScreenResolutionChangeFailedDialog();
	}

	delete pInfo;
}

static
void sUIScreenResolutionConfirmDialogScheduledEventCallback(GAME * game, const CEVENT_DATA & data, DWORD /*???*/)
{
	UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO * pInfo = reinterpret_cast<UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO *>(data.m_Data1);

	// This event is kaput.
	pInfo->nScheduledEvent = INVALID_ID;

	// Compute time remaining.
	// CHB 2007.06.29 - Changelist 43026, which eliminates the
	// arithmetic operators for the TIME type in ptime.h, broke
	// this computation.
	int nSecondsRemaining = static_cast<int>(static_cast<TIMEDELTA>(pInfo->nEndTime - AppCommonGetAbsTime()) / 1000);

	// Time up?
	if (nSecondsRemaining >= 0)
	{
		// Keep going with a new, improved event.
		pInfo->nScheduledEvent = CSchedulerRegisterEventImm(&sUIScreenResolutionConfirmDialogScheduledEventCallback, data);

		// Time display update?
		if (nSecondsRemaining != pInfo->nLastSeconds)
		{
			pInfo->nLastSeconds = nSecondsRemaining;

			//
			// Update text on dialog.
			//

			// Get dialog.
			UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
			if (pDialog == 0)
			{
				return;
			}

			// Get dialog text label.
			UI_COMPONENT * pLabel = UIComponentFindChildByName(pDialog, "generic dialog text");
			if (pLabel == 0)
			{
				return;
			}
			if (pLabel->m_eComponentType != UITYPE_LABEL)
			{
				return;
			}

			// Create text string showing countdown time.
			const WCHAR * pText = GlobalStringGet(GS_UI_SCREEN_RESOLUTION_CHANGE_CONFIRM_TEXT);
			int nSize = PStrLen(pText) + 8;
			std::auto_ptr<WCHAR> pNewText(new WCHAR[nSize]);
			PStrPrintf(pNewText.get(), nSize, L"%s (%d)", pText, nSecondsRemaining);

			// Update the label.
			UILabelSetText(pLabel, pNewText.get());

			if (UIComponentIsTooltip(pDialog))
			{
				UI_TOOLTIP * pTooltipDialog = UICastToTooltip(pDialog);
				UITooltipDetermineContents(pTooltipDialog);
				UITooltipDetermineSize(pTooltipDialog);
			}

		}
	}
	else
	{
		UIHideGenericDialog();
	}
}


//----------------------------------------------------------------------------
//
// SOUND OPTIONS
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISpeakerConfigOnPostCreate( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	
	UI_COMBOBOX *pCombo = UICastToComboBox(component);

	for (int i = 0; i < nSoundSpeakerMetaCount; i++)
	{
		const WCHAR *wszStr = StringTableGetStringByKey(SoundSpeakerMeta[i].pszName);
		UIListBoxAddString(pCombo->m_pListBox, wszStr, SoundSpeakerMeta[i].nData);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISpeakerConfigOnSelect( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	
	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//
// GENERAL OPTIONS DIALOG
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScaleOnPostCreate( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_COMBOBOX *pCombo = UICastToComboBox(component);

	float fScale = 1.0f;
	for (int i = 0; i < 21; i++)
	{
		const int MAX_STRING = 64;
		WCHAR Str[MAX_STRING];
		LanguageFormatFloatString( Str, MAX_STRING, fScale, 2 );
		UIListBoxAddString(pCombo->m_pListBox, Str);
		fScale += .05f;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScaleOnSelect( 
									  UI_COMPONENT* component, 
									  int msg, 
									  DWORD wParam, 
									  DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUICopyFromFeatureControlsToState(void)
{
	ASSERT_RETURN( s_pWorkingFeatureMap );
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	for (unsigned i = 0; i < _countof(s_aFeatureCombos); ++i)
	{
		int nIndex = UIComboBoxGetSelectedIndex(pDialog, s_aFeatureCombos[i].pComboName);
		s_pWorkingFeatureMap->SelectChoice(s_aFeatureCombos[i].nLineName, nIndex);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
UI_MSG_RETVAL sUIOptionsComboOnClick(UI_COMPONENT * component, int msg, DWORD wParam, DWORD lParam)
{
	UI_MSG_RETVAL nResult = UIMSG_RET_HANDLED;

	// Oh, and by the way, updating the dialog controls will cause
	// this 'on select' function to be called. So here, stifle the
	// impending infinite recursion that would otherwise ensue.
	// Although, to bore the reader further, I shall say that the
	// term "unbounded recursion" is preferred, as truly infinite
	// recursion does not exist, except possibly in the minds of
	// computer science textbook authors.
	static bool bInRefresh = false;
	if (!bInRefresh)
	{
		bInRefresh = true;

		//
		UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
		if (pDialog != 0)
		{
			// Close all other combos first.
			for (int i = 0; i < _countof(s_aComboList); ++i)
			{
				UI_COMPONENT * pComboComp = UIComponentFindChildByName(pDialog, s_aComboList[i]);
				if (pComboComp != 0)
				{
					UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
					if ((pCombo != 0) && (pCombo->m_pButton != component))
					{
						sUIComboClose(pCombo);
					}
				}
			}
		}
		
		//
		nResult = UIWindowshadeButtonOnClick(component, msg, wParam, lParam);

		// The setting changed could indirectly affect other settings.
		// So, update the state from the controls here. The act of
		// updating the state will cause all settings to be validated
		// and restricted as appropriate. Then re-update the dialog
		// controls with the validated state. The result is immediate
		// feedback to the user of settings interdependency, the Holy
		// Grail made possible by -- and the raison d'être of -- the
		// OptionState system.
		sUICopyFromFeatureControlsToState();
		sUIRefreshDialog();

		bInRefresh = false;
	}
	else
	{
		//
		nResult = UIWindowshadeButtonOnClick(component, msg, wParam, lParam);
	}

	return nResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIComboInit(UI_COMPONENT * pComponent, bool bInit)
{
	PFN_UIMSG_HANDLER pFrom = bInit ? &UIWindowshadeButtonOnClick : &sUIOptionsComboOnClick;
	PFN_UIMSG_HANDLER pTo = bInit ? &sUIOptionsComboOnClick : &UIWindowshadeButtonOnClick;
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);

	UI_MSGHANDLER *pHandler =  UIGetMsgHandler(pCombo->m_pButton, UIMSG_LBUTTONCLICK);

	if (pHandler)
	{
		if (pHandler->m_fpMsgHandler == pFrom)
		{
			pHandler->m_fpMsgHandler = pTo;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIInitOptionsCombos(UI_COMPONENT * pComponent, bool bInit)
{
	for (unsigned i = 0; i < _countof(s_aComboList); ++i)
	{
		UI_COMPONENT * pCombo = UIComponentFindChildByName(pComponent, s_aComboList[i]);
		if (pCombo != 0)
		{
			sUIComboInit(pCombo, bInit);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
bool sShouldShowConfirmDialog(const OptionState * pOld, const OptionState * pNew)
{
	// Not necessary for windowed changes.
	if (pNew->get_bWindowed())
	{
		return false;
	}

	// This just compares displaymode which will work for now.
	// But a better way may be to confirm for anything that requires
	// a new device.
	E_SCREEN_DISPLAY_MODE OldMode, NewMode;
	e_Screen_DisplayModeFromState(pOld, OldMode);
	e_Screen_DisplayModeFromState(pNew, NewMode);
	return
		(pOld->get_nFrameBufferWidth() != pNew->get_nFrameBufferWidth()) ||
		(pOld->get_nFrameBufferHeight() != pNew->get_nFrameBufferHeight()) ||
		(OldMode.RefreshRate != NewMode.RefreshRate) ||
		pOld->get_bWindowed();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void s_UIReloadCallback(void)
{
	if (sShouldShowConfirmDialog(s_pOriginalState, s_pWorkingState))
	{
		//GfxOptions_UpdateFromEngine();

		// Make a persistent copy. It will be deleted when the
		// confirmation dialog is complete.
		UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO * pInfo = new UI_SCREEN_RESOLUTION_CONFIRM_DIALOG_INFO;

		// Can't use a timed event since the scheduler is "paused" along with the game.
		pInfo->nEndTime = AppCommonGetAbsTime() + 15 * 1000;
		pInfo->nScheduledEvent = CSchedulerRegisterEventImm(&sUIScreenResolutionConfirmDialogScheduledEventCallback, CEVENT_DATA(reinterpret_cast<DWORD_PTR>(pInfo)));

		// Screen change succeeded; show confirmation dialog.
		DIALOG_CALLBACK OkCallback;
		OkCallback.pfnCallback = &sUIScreenResolutionConfirmDialogOkCallback;
		OkCallback.pCallbackData = pInfo;
		DIALOG_CALLBACK CancelCallback;
		CancelCallback.pfnCallback = &sUIScreenResolutionConfirmDialogCancelCallback;
		CancelCallback.pCallbackData = pInfo;
		UIShowGenericDialog(GlobalStringGet(GS_UI_SCREEN_RESOLUTION_CHANGE_CONFIRM_TITLE), GlobalStringGet(GS_UI_SCREEN_RESOLUTION_CHANGE_CONFIRM_TEXT), TRUE, &OkCallback, &CancelCallback, AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);
		
		if( AppIsTugboat() )
		{
			UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
			UISetMouseOverrideComponent(pDialog);
			UISetKeyboardOverrideComponent(pDialog);
		}
	}
	else
	{
		sCleanupState();
		UIRestoreLastMenu();
		GfxOptions_Save();
		sSaveAspectRatioSelection();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.05.10 - Use this function to bring up the options dialog
// with the existing settings.
static
void sUIActivateOptionsDialog(void)
{
	UIComponentActivate(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), TRUE);
	sUIRefreshDialog();	// CHB 2007.08.16 - need to refresh after activation, since activation can change the visibility state of child components
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIExitAppConfirmDialogOkCallback(void * /*pContext*/, DWORD /*dwCallbackData*/)
{
	// Save settings and exit.
	GfxOptions_Save(s_pWorkingState, s_pWorkingFeatureMap);
	sCleanupState();
	UIExitApplication();
}

static
void sUIExitAppConfirmDialogCancelCallback(void * /*pContext*/, DWORD /*dwCallbackData*/)
{
	sUIActivateOptionsDialog();
}

static
void sUIShowExitAppDialog(void)
{
	// Screen change succeeded; show confirmation dialog.
	DIALOG_CALLBACK OkCallback;
	OkCallback.pfnCallback = &sUIExitAppConfirmDialogOkCallback;
	OkCallback.pCallbackData = 0;
	DIALOG_CALLBACK CancelCallback;
	CancelCallback.pfnCallback = &sUIExitAppConfirmDialogCancelCallback;
	CancelCallback.pCallbackData = 0;
	UIShowGenericDialog(
		GlobalStringGet(GS_UI_OPTIONS_REQUIRE_APP_EXIT_TITLE),
		GlobalStringGet(GS_UI_OPTIONS_REQUIRE_APP_EXIT_TEXT),
		TRUE,
		&OkCallback,
		&CancelCallback
	);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct FlagOptionData
{
	const char * pControlName;
	bool (OptionState::* pGetFunction)(void) const;
	void (OptionState::* pSetFunction)(const bool b);
};

static
const FlagOptionData FlagOptions[] =
{
	// Put boolean options and their check boxes here.
	// Then their saving and retrieval will happen automagically!
	{	UI_WINDOWED_BUTTON,			&OptionState::get_bWindowed,				&OptionState::set_bWindowed					},
	{	"vsync btn",				&OptionState::get_bFlipWaitVerticalRetrace,	&OptionState::set_bFlipWaitVerticalRetrace	},
	{	"anisotropic btn",			&OptionState::get_bAnisotropic,				&OptionState::set_bAnisotropic				},
	{	"triple buffer btn",		&OptionState::get_bTripleBuffer,			&OptionState::set_bTripleBuffer				},
	{	"dynamic lights btn",		&OptionState::get_bDynamicLights,			&OptionState::set_bDynamicLights			},
	{	"trilinear btn",			&OptionState::get_bTrilinear,				&OptionState::set_bTrilinear				},
	{	"async effects btn",		&OptionState::get_bAsyncEffects,			&OptionState::set_bAsyncEffects				},
	{	UI_FLUID_EFFECTS_BUTTON,	&OptionState::get_bFluidEffects,			&OptionState::set_bFluidEffects				},
};

static
void sMoveFlagOptions(UI_COMPONENT * pDialog, OptionState * pState, bool bSave)
{
	if (bSave)
	{
		pState->SuspendUpdate();
	}

	for (unsigned i = 0; i < _countof(FlagOptions); ++i)
	{
		if (bSave)
		{
			(pState->*FlagOptions[i].pSetFunction)(!!UIButtonGetDown(pDialog, FlagOptions[i].pControlName));
		}
		else
		{
			UIButtonSetDown(pDialog, FlagOptions[i].pControlName, (pState->*FlagOptions[i].pGetFunction)());
		}
	}

	if (bSave)
	{
		pState->ResumeUpdate();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsDialogAccept( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!UIComponentGetActive(component))	
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// *** write all the changed options
	UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	sgtOptions.tSoundOptions.nSpeakerConfig = UIComboBoxGetSelectedIndex(pDialog, "speaker config combo");
	sgtOptions.tSoundOptions.fSoundVolume = UIScrollBarGetValue(pDialog, "sound volume slider", TRUE);
	sgtOptions.tSoundOptions.fUIVolume = UIScrollBarGetValue(pDialog, "ui volume slider", TRUE);
	sgtOptions.tMusicOptions.fMusicVolume = UIScrollBarGetValue(pDialog, "music volume slider", TRUE);
	sgtOptions.tMusicOptions.bMuteMusic = !!UIButtonGetDown(pDialog, "music mute btn");
	sgtOptions.tSoundOptions.bMuteSound = !!UIButtonGetDown(pDialog, "sound mute btn");

	sgtOptions.tSoundOptions.bDisableReverb = !!UIButtonGetDown(pDialog, "disable reverb btn");
	//sgtOptions.tSoundOptions.bDisableSoftwareFX = !!UIButtonGetDown(pDialog, "disable software fx btn");
	sgtOptions.tSoundOptions.bEnableHardwareReverb = false;
	//sgtOptions.tSoundOptions.bEnableHardwareReverb = !!UIButtonGetDown(pDialog, "enable hardware reverb btn");

	// *** xfire login options
	PStrCvt(sgtOptions.tSoundOptions.pszXfireUsername, UILabelGetTextByEnum(UICOMP_XFIRE_USERNAME), arrsize(sgtOptions.tSoundOptions.pszXfireUsername));
	PStrCvt(sgtOptions.tSoundOptions.pszXfirePassword, UILabelGetTextByEnum(UICOMP_XFIRE_PASSWORD), arrsize(sgtOptions.tSoundOptions.pszXfirePassword));
	sgtOptions.tSoundOptions.bStartXfire = !!UIButtonGetDown(pDialog, "xfire autologin btn");
	sgtOptions.tSoundOptions.bEnableVoiceChat = !!UIButtonGetDown(pDialog, "xfire enable voicechat btn");
	//

	sgtOptions.tControlOptions.bShiftAsModifier = !!UIButtonGetDown(pDialog, "shift as modifier btn");
	sgtOptions.tControlOptions.bCtrlAsModifier = !!UIButtonGetDown(pDialog, "ctrl as modifier btn");
	sgtOptions.tControlOptions.bAltAsModifier = !!UIButtonGetDown(pDialog, "alt as modifier btn");

	c_SoundSetSoundConfigOptions(&sgtOptions.tSoundOptions);	
	c_SoundSetMusicConfigOptions(&sgtOptions.tMusicOptions);	

	// CHB 2007.03.16 - Find alternatives.
	//sgtOptions.tGfxOptions.fGfxSlider = UIScrollBarGetValue(pDialog, "gfx features slider", TRUE);
	//s_pWorkingState->set_fTextureQuality(UIScrollBarGetValue(pDialog, "texture detail slider", TRUE));
	s_pWorkingState->set_fGammaPower(e_MapGammaPctToPower(UIScrollBarGetValue(pDialog, UI_GAMMA_SLIDER, TRUE)));

	// CHB 2007.07.18
	sMoveFlagOptions(pDialog, s_pWorkingState, true);

	sgtOptions.tGameOptions.bCensorshipEnabled = !!UIButtonGetDown(pDialog, "hide blood btn");
	//sgtOptions.tGameOptions.bAltTargetInfo = !!UIButtonGetDown(pDialog, "alt target info btn");
	//sgtOptions.tGameOptions.bAltTargeting = !!UIButtonGetDown(pDialog, "alt targeting btn");
	sgtOptions.tGameOptions.bInvertMouse = !!UIButtonGetDown(pDialog, "invert mouse btn");
	sgtOptions.tGameOptions.bRotateAutomap = !!UIButtonGetDown(pDialog, "rotate automap btn");
	sgtOptions.tGameOptions.bEnableCinematicSubtitles = !!UIButtonGetDown(pDialog, "enable cinematic subtitles btn");
	sgtOptions.tGameOptions.bDisableAdUpdates = !!UIButtonGetDown(pDialog, "disable ad updates btn");
	sgtOptions.tGameOptions.bChatAutoFade = !!UIButtonGetDown(pDialog, "chat auto fade btn");
	sgtOptions.tGameOptions.bShowChatBubbles = !!UIButtonGetDown(pDialog, "show chat bubbles btn");
	sgtOptions.tGameOptions.bAbbreviateChatName = !!UIButtonGetDown(pDialog, "abbreviate chat name btn");
	if( AppIsHellgate())
	{
		UIChatSetAutoFadeState(sgtOptions.tGameOptions.bChatAutoFade);
	}

	UI_COMPONENT *pScrollbar = NULL;
	//pScrollbar = UIComponentFindChildByName(pDialog,"mouse sensitivity slider");
	//if (pScrollbar)
	//{
	//	UI_SCROLLBAR *pBar = UICastToScrollBar(pScrollbar);
	//	sgtOptions.tGameOptions.fMouseSensitivity = MAP_VALUE_TO_RANGE(UIScrollBarGetValue(pBar, TRUE), 0.0f, 1.0f, MOUSE_CURSOR_SENSITIVITY_MIN, MOUSE_CURSOR_SENSITIVITY_MAX);
	//}
	pScrollbar = UIComponentFindChildByName(pDialog,"mouse look sensitivity slider");
	if (pScrollbar)
	{
		UI_SCROLLBAR *pBar = UICastToScrollBar(pScrollbar);
		sgtOptions.tGameOptions.fMouseLookSensitivity = MAP_VALUE_TO_RANGE(UIScrollBarGetValue(pBar, TRUE), 0.0f, 1.0f, MOUSE_LOOK_SENSITIVITY_MIN, MOUSE_LOOK_SENSITIVITY_MAX);
	}

	pScrollbar = UIComponentFindChildByName(pDialog,"chat window transparency slider");
	if (pScrollbar)
	{
		UI_SCROLLBAR *pBar = UICastToScrollBar(pScrollbar);
		sgtOptions.tGameOptions.nChatAlpha = (int)MAP_VALUE_TO_RANGE(UIScrollBarGetValue(pBar, TRUE), 0.0f, 1.0f, CHAT_TRANSPARENCY_MIN, CHAT_TRANSPARENCY_MAX);
	}


	pScrollbar = UIComponentFindChildByName(pDialog,"map transparency slider");
	if (pScrollbar)
	{
		UI_SCROLLBAR *pBar = UICastToScrollBar(pScrollbar);
		sgtOptions.tGameOptions.nMapAlpha = (int)MAP_VALUE_TO_RANGE(UIScrollBarGetValue(pBar, TRUE), 0.0f, 1.0f, CHAT_TRANSPARENCY_MIN, CHAT_TRANSPARENCY_MAX);
	}

	if( AppIsTugboat() )
	{
		sgtOptions.tGameOptions.nUIScaler = UIComboBoxGetSelectedIndex(pDialog, "UI scale combo");

		sgtOptions.tGameOptions.bAdvancedCamera = !!UIButtonGetDown(pDialog, "advanced camera btn");
		sgtOptions.tGameOptions.bLockedPitch = !!UIButtonGetDown(pDialog, "lock pitch btn");
		sgtOptions.tGameOptions.bFollowCamera = !!UIButtonGetDown(pDialog, "follow camera btn");

	}
	else
	{
		sgtOptions.tGameOptions.nUIScaler = 0;
	}

	// CHB 2007.03.12 - Ideally they're updated as the combo
	// selection changes, but we also get them here as a
	// catch-all.
	sUICopyFromFeatureControlsToState();

	// Save game config.
	GameOptions_Set(sgtOptions.tGameOptions);

	// Save key config.
	ControlOptions_Set(sgtOptions.tControlOptions);
	UIHandleUIMessage(UIMSG_INPUTLANGCHANGE, 0, 0); // update text labels to reflect any key changes

	// Done with options.
	sLatchAspectRatioSelection();	// CHB 2007.08.15
	sAcceptEngineVersion();			// CHB 2007.08.16
	UIHideOptionsDialog();

	// CHB 2007.05.10 - Check to see if the changes require exiting the game.
	unsigned nAction = e_OptionState_QueryActionRequired(e_OptionState_GetActive(), s_pWorkingState);
	if ((nAction & SETTING_NEEDS_APP_EXIT) != 0)
	{
		sUIShowExitAppDialog();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	// If the screen res changed, show a dialog box confirming the new mode
	// for up to 15 seconds.
	UISetReloadCallback(&s_UIReloadCallback);	// CHB 2007.04.03
	if (FAILED(sCommitWorkingState()))
	{
		// Screen change failed; show error dialog.
		UISetReloadCallback(0);	// CHB 2007.04.03
		UIShowScreenResolutionChangeFailedDialog();
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.05.10 - Process of getting state for display in the options
// dialog is separated out so that we can activate the dialog later
// without affecting the settings.
static
void sUICopyFromStateToControls(void)
{
	GameOptions_Get(sgtOptions.tGameOptions);	// CHB 2007.03.06
	ControlOptions_Get(sgtOptions.tControlOptions);	// CHB 2007.03.06
	c_SoundGetSoundConfigOptions(&sgtOptions.tSoundOptions);
	c_SoundGetMusicConfigOptions(&sgtOptions.tMusicOptions);
	MemoryCopy(&sgtOptions.tSoundOptionsOriginal, sizeof(SOUND_CONFIG_OPTIONS), &sgtOptions.tSoundOptions, sizeof(SOUND_CONFIG_OPTIONS));
	MemoryCopy(&sgtOptions.tMusicOptionsOriginal, sizeof(MUSIC_CONFIG_OPTIONS), &sgtOptions.tMusicOptions, sizeof(MUSIC_CONFIG_OPTIONS));

	if (!sInitState())
	{
		// ?
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sGraphicOptions(UI_COMPONENT * component)
{
	UIScrollBarSetValue(component, UI_GAMMA_SLIDER, e_MapGammaPowerToPct( s_pWorkingState->get_fGammaPower() ), TRUE);

	sMoveFlagOptions(component, s_pWorkingState, false);

	// Initialize screen resolution and refresh rate combos.
	UISnapshotScreenResolutionList();
	const E_SCREEN_DISPLAY_MODE & Mode = sUIGetCurrentScreenResolution();

	// CHB 2007.08.15 - Defaulting to 'ALL' for the first time the options are brought up
	//const ASPECT_RATIO_TYPE nAR = sUIGetAspectRatio(Mode);
	const ASPECT_RATIO_TYPE nAR = s_nSelectedAspectRatio;
	UI_COMPONENT * pCombo = UIComponentFindChildByName(component, UI_SCREEN_ASPECT_RATIO_COMBO);
	if (pCombo != 0)
	{
		sUIScreenAspectRatioComboSetSelection(pCombo, nAR);
	}

	pCombo = UIComponentFindChildByName(component, UI_SCREEN_RESOLUTION_COMBO);
	if (pCombo != 0)
	{
		// In Mythos - we just use ALL aspect ratios, instead of breaking up the lists
		//sUIScreenResolutionComboSetSelection(pCombo, Mode, AppIsTugboat() ? ASPECT_RATIO_ALL : nAR);
		// CHB 2007.08.15 - I think the new defaulting to all will accomplish this acceptably for Mythos.
		sUIScreenResolutionComboSetSelection(pCombo, Mode, nAR);
	}

	pCombo = UIComponentFindChildByName(component, UI_SCREEN_REFRESH_COMBO);
	if (pCombo != 0)
	{
		sUIScreenRefreshComboSetSelection(pCombo, Mode);
	}

	sUIUpdateGraphicsOptionsUIState();	// CHB 2007.02.01
	sUIRefreshDialog();		// CHB 2007.03.05
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

	// On DX9, hide the fluid effects checkbox.
	if( AppIsHellgate() )
	{
		UI_COMPONENT * pCheck = UIComponentFindChildByName(component, UI_FLUID_EFFECTS_BUTTON);
		UI_COMPONENT * pLabel = UIComponentFindChildByName(component, UI_FLUID_EFFECTS_LABEL);
		sUIDoWhatIWant(pCheck, !!e_DX10IsEnabled());
		sUIDoWhatIWant(pLabel, !!e_DX10IsEnabled());
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.06.29 - Note: Oddly, this function is actually called
// BEFORE UIOptionsDialogOnPostActivate() the first time the dialog
// is activated.
UI_MSG_RETVAL UIGraphicOptionsDialogOnPostActivate(
	UI_COMPONENT* component,
	int /*msg*/,
	DWORD /*wParam*/,
	DWORD /*lParam*/ )
{
	sGraphicOptions(component);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
UI_MSG_RETVAL UIOptionsGraphicsDefaults(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// CHB 2007.10.22 - No confirm dialog issued here as the
	// user can just select cancel/accept as appropriate.
	CComPtr<OptionState> pState;
	CComPtr<FeatureLineMap> pMap;
	GfxOptions_GetDefaults(&pState, &pMap);
	if ((pState != 0) && (pMap != 0))
	{
		V(e_OptionState_CopyDeferrablesFromActive(pState));

		s_pWorkingFeatureMap = pMap;
		s_pWorkingState = pState;

		// Now refresh the dialog.
		UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
		if (pDialog != 0)
		{
			sUIRefreshDialog();
			sGraphicOptions(pDialog);
			UIComponentHandleUIMessage(pDialog, UIMSG_PAINT, 0, 0);
			sSetGammaFromSlider();
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sUIOptionsUnmapModifierKey(
	const int vKey,
	const BOOL bAsModifier,
	const BOOL bUnmapNow)
{
	BOOL bFound = FALSE;

	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	{
		const KEY_COMMAND& tKeyCheck = InputGetKeyCommand(i); //sgtOptions.tControlOptions.pKeyAssignments[i];
		if (tKeyCheck.eGame == AppGameGet() || tKeyCheck.eGame == APP_GAME_ALL)
		{
			for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
			{
				if (sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nModifierKey == vKey)
				{
					if (bAsModifier)
					{
						if (sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nKey == vKey)
						{
							continue;
						}
					}
					else
					{
						if (sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nKey != vKey)
						{
							continue;
						}
					}

					if (bUnmapNow)
					{
						sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nModifierKey = 0;
						sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nKey = 0;
						bFound = TRUE;
					}
					else
					{
						return TRUE;
					}					
				}
			}
		}
	}

	if (bFound)
	{
		UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), UIMSG_PAINT, 0, 0);
	}

	return bFound;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sKeyUnmapCallbackOk(
	void * component, 
	DWORD dwData )
{
	ASSERT_RETURN(component);

	const int vKey = (int)(dwData & 0x7fffffff);
	const BOOL bAsModifier = (dwData & ((DWORD)1<<31)) != 0;

	sUIOptionsUnmapModifierKey(vKey, bAsModifier, TRUE);

	UI_BUTTONEX *pButton = (UI_BUTTONEX*)component;
	UIButtonSetDown(pButton, !bAsModifier);
	UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
UI_MSG_RETVAL UIAsModifierButtonOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_BUTTONEX *pButton = UICastToButton(component);
	BOOL bIsDown = !UIButtonGetDown(pButton);

	// Find vk code associated with this button
	if (component->m_szName)
	{
		int vKey;
		switch (component->m_szName[0])
		{
			case 's': vKey = VK_SHIFT;   break;
			case 'c': vKey = VK_CONTROL; break;
			case 'a': vKey = VK_MENU;    break;
			default: return UIMSG_RET_NOT_HANDLED;
		};

		if( AppIsHellgate() )
		{
			if (bIsDown)
			{
				// setting to "as modifier" - need to make sure the key is not mapped as a stand-alone key.
				// if it is, we need to warn the player and then un-set it.
				if (sUIOptionsUnmapModifierKey(vKey, FALSE, FALSE))
				{
					DIALOG_CALLBACK tOkCallback;
					DialogCallbackInit( tOkCallback );
					tOkCallback.pfnCallback = sKeyUnmapCallbackOk;
					tOkCallback.pCallbackData = (void*)component;
					tOkCallback.dwCallbackData = vKey;
					UIShowGenericDialog(GlobalStringGet(GS_CONFIRM_DIALOG_HEADER), GlobalStringGet(GS_AS_MODIFIER_ON_CONFIRMATION), TRUE, &tOkCallback, 0, AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
			else
			{
				// un-setting "as modifier" - need to make sure the key is not mapped as a modifier key.
				// if it is, we need to warn the player and then un-set it.
				if (sUIOptionsUnmapModifierKey(vKey, TRUE, FALSE))
				{
					DIALOG_CALLBACK tOkCallback;
					DialogCallbackInit( tOkCallback );
					tOkCallback.pfnCallback = sKeyUnmapCallbackOk;
					tOkCallback.pCallbackData = (void*)component;
					tOkCallback.dwCallbackData = ((DWORD)1<<31) | vKey;
					UIShowGenericDialog(GlobalStringGet(GS_CONFIRM_DIALOG_HEADER), GlobalStringGet(GS_AS_MODIFIER_OFF_CONFIRMATION), TRUE, &tOkCallback, 0, AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
		}

	}

	UIButtonSetDown(pButton, bIsDown);
	UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsDialogOnPostActivate( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	// close any possibly interfering UI that might still be open
	UIQuickClose();

	// initialize controls

	UIComboBoxSetSelectedIndex(component, "speaker config combo", sgtOptions.tSoundOptions.nSpeakerConfig);
	UIScrollBarSetValue(component, "sound volume slider", sgtOptions.tSoundOptions.fSoundVolume, TRUE);
	UIScrollBarSetValue(component, "music volume slider", sgtOptions.tMusicOptions.fMusicVolume, TRUE);
	UIScrollBarSetValue(component, "ui volume slider", sgtOptions.tSoundOptions.fUIVolume, TRUE);
	UIButtonSetDown(component, "music mute btn", sgtOptions.tMusicOptions.bMuteMusic);
	UIButtonSetDown(component, "sound mute btn", sgtOptions.tSoundOptions.bMuteSound);

	UIButtonSetDown(component, "disable reverb btn", sgtOptions.tSoundOptions.bDisableReverb);
	//UIButtonSetDown(component, "disable software fx btn", sgtOptions.tSoundOptions.bDisableSoftwareFX);
	//UIButtonSetDown(component, "enable hardware reverb btn", sgtOptions.tSoundOptions.bEnableHardwareReverb);

	//UIButtonSetDown(component, "alt target info btn", sgtOptions.tGameOptions.bAltTargetInfo);
	//UIButtonSetDown(component, "alt targeting btn", sgtOptions.tGameOptions.bAltTargeting);
	UIButtonSetDown(component, "invert mouse btn", sgtOptions.tGameOptions.bInvertMouse);
	UIButtonSetDown(component, "rotate automap btn", sgtOptions.tGameOptions.bRotateAutomap);
	UIButtonSetDown(component, "enable cinematic subtitles btn", sgtOptions.tGameOptions.bEnableCinematicSubtitles);
	UIButtonSetDown(component, "disable ad updates btn", sgtOptions.tGameOptions.bDisableAdUpdates);
	UIButtonSetDown(component, "chat auto fade btn", sgtOptions.tGameOptions.bChatAutoFade);
	UIButtonSetDown(component, "show chat bubbles btn", sgtOptions.tGameOptions.bShowChatBubbles);
	UIButtonSetDown(component, "abbreviate chat name btn", sgtOptions.tGameOptions.bAbbreviateChatName);

	UIButtonSetDown(component, "hide blood btn", sgtOptions.tGameOptions.bCensorshipEnabled);
//	UIScrollBarSetValue(component, "mouse sensitivity slider", MAP_VALUE_TO_RANGE(sgtOptions.tGameOptions.fMouseSensitivity, MOUSE_CURSOR_SENSITIVITY_MIN, MOUSE_CURSOR_SENSITIVITY_MAX, 0.0f, 1.0f), TRUE);
	UIScrollBarSetValue(component, "mouse look sensitivity slider", MAP_VALUE_TO_RANGE(sgtOptions.tGameOptions.fMouseLookSensitivity, MOUSE_LOOK_SENSITIVITY_MIN, MOUSE_LOOK_SENSITIVITY_MAX, 0.0f, 1.0f), TRUE);
	UIScrollBarSetValue(component, "chat window transparency slider", MAP_VALUE_TO_RANGE(sgtOptions.tGameOptions.nChatAlpha, CHAT_TRANSPARENCY_MIN, CHAT_TRANSPARENCY_MAX, 0.0f, 1.0f), TRUE);
	if( AppIsTugboat() )
	{
		UIScrollBarSetValue(component, "map transparency slider", MAP_VALUE_TO_RANGE(sgtOptions.tGameOptions.nMapAlpha, CHAT_TRANSPARENCY_MIN, CHAT_TRANSPARENCY_MAX, 0.0f, 1.0f), TRUE);
		UIButtonSetDown(component, "advanced camera btn", sgtOptions.tGameOptions.bAdvancedCamera);
		UIButtonSetDown(component, "lock pitch btn", sgtOptions.tGameOptions.bLockedPitch);
		UIButtonSetDown(component, "follow camera btn", sgtOptions.tGameOptions.bFollowCamera);

	}

	UIButtonSetDown(component, "xfire autologin btn", sgtOptions.tSoundOptions.bStartXfire);
	UIButtonSetDown(component, "xfire enable voicechat btn", sgtOptions.tSoundOptions.bEnableVoiceChat);
	WCHAR pszXfireUsernameW[arrsize(sgtOptions.tSoundOptions.pszXfireUsername)];
	WCHAR pszXfirePasswordW[arrsize(sgtOptions.tSoundOptions.pszXfirePassword)];
	PStrCvt(pszXfireUsernameW, sgtOptions.tSoundOptions.pszXfireUsername, arrsize(pszXfireUsernameW));
	PStrCvt(pszXfirePasswordW, sgtOptions.tSoundOptions.pszXfirePassword, arrsize(pszXfirePasswordW));
	UILabelSetTextByEnum(UICOMP_XFIRE_USERNAME, pszXfireUsernameW);
	UILabelSetTextByEnum(UICOMP_XFIRE_PASSWORD, pszXfirePasswordW);
	UIOptionsXfireSetStatus(c_VoiceChatIsLoggedIn(), FALSE);

	UIButtonSetDown(component, "shift as modifier btn", sgtOptions.tControlOptions.bShiftAsModifier);
	UIButtonSetDown(component, "ctrl as modifier btn", sgtOptions.tControlOptions.bCtrlAsModifier);
	UIButtonSetDown(component, "alt as modifier btn", sgtOptions.tControlOptions.bAltAsModifier);

	if (sgtOptions.tSoundOptions.bEnableVoiceChat)
		c_VoiceChatStart();
	//

	if( AppIsTugboat() )
	{
		UIComboBoxSetSelectedIndex(component, "UI scale combo", sgtOptions.tGameOptions.nUIScaler);
	}

	UI_COMPONENT * pAdDisableButton = UIComponentFindChildByName(component, UI_DISABLE_AD_CLIENT_BUTTON);
	UI_COMPONENT * pAdDisableLabel = UIComponentFindChildByName(component, UI_DISABLE_AD_CLIENT_LABEL);
	if(AppIsSinglePlayer() && (!AppIsBeta() && !AppIsDemo()))
	{
		if(pAdDisableButton)
		{
			UIComponentActivate(pAdDisableButton, TRUE);
		}
		if(pAdDisableLabel)
		{
			UIComponentActivate(pAdDisableLabel, TRUE);
		}
	}
	else
	{
		if(pAdDisableButton)
		{
			UIComponentActivate(pAdDisableButton, FALSE);
		}
		if(pAdDisableLabel)
		{
			UIComponentActivate(pAdDisableLabel, FALSE);
		}
	}

	UI_COMPONENT * pGameAccountPanel = UIComponentFindChildByName(component, UI_GAME_ACCOUNT_PANEL);
	if (pGameAccountPanel)
	{
		UIComponentSetVisible(pGameAccountPanel, !AppGetCltGame() || AppIsMultiplayer());
		if (pGameAccountPanel->m_bVisible)
		{
			UIComponentActivate(pGameAccountPanel, TRUE);

			//UI_COMPONENT * pTimeRemainingLabel = UIComponentFindChildByName(component, UI_ACCOUNT_TIME_REMAINING_LABEL);
			//if (pTimeRemainingLabel)
			//{
			//	UI_LABELEX *pLabel = UICastToLabel(pTimeRemainingLabel);
			//	UILabelSetText(pLabel, L"");
			//	if (AppGetCltGame() && AppIsMultiplayer() && !c_BillingHasPendingRequestAccountStatus())
			//	{
			//		c_BillingSendMsgRequestAccountStatus();
			//	}
			//}

			//UI_COMPONENT * pAccountStatusLabel = UIComponentFindChildByName(component, UI_ACCOUNT_STATUS_LABEL);
			//if (pAccountStatusLabel)
			//{
			//	UI_LABELEX *pLabel = UICastToLabel(pAccountStatusLabel);
			//	UILabelSetText(pLabel, L"");
			//	if (AppGetCltGame() && AppIsMultiplayer())
			//	{
			//		UIUpdateAccountStatus(pLabel);
			//	}
			//}
		}
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

	UISetMouseOverrideComponent(component);
	UISetKeyboardOverrideComponent(component);

	sPopulateEngineVersionCombo();	// CHB 2007.08.16

	// CHB 2007.05.23 - Move this down here, and add a
	// deinitialization later, so that control initialization
	// doesn't trigger the combo box click routine, which updates
	// the combos by saving the combo selections to the working
	// state (which enforces any restrictions and modifies the
	// state accordingly), then copying the state back to the
	// combos.  However, during initialization, the combos don't
	// have valid selections, so the "saving the combo selections
	// to the working state" part of that would backfire.
	sUIInitOptionsCombos(component, true);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsDialogOnPostInactivate( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	sUIInitOptionsCombos(component, false);	// CHB 2007.05.23

	UIRemoveMouseOverrideComponent(component);
	UIRemoveKeyboardOverrideComponent(component);
	InputEndKeyRemap();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowOptionsDialog(void)
{
	UIMenuSetSelect(0);
	UIHideCurrentMenu();
	UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);
	UIStopAllSkillRequests();
	c_PlayerClearAllMovement(AppGetCltGame());

	sUICopyFromStateToControls();	// CHB 2007.05.10 - Bringing up the dialog, get the current settings
	sUIActivateOptionsDialog();		// CHB 2007.05.10 - Show the dialog
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideOptionsDialog(void)
{
	//UIComponentSetVisible(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), FALSE, TRUE);
	UIComponentActivate(UICOMP_OPTIONSDIALOG, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsDialogCancel( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!UIComponentGetActive(component))	
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP) &&
		wParam != VK_ESCAPE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if( AppIsHellgate() )
	{
		UI_COMPONENT * pDialogBox = UIComponentGetByEnum(UICOMP_DIALOGBOX);
		if (UIComponentGetActive(pDialogBox))	
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

	c_SoundApplySettings(&sgtOptions.tSoundOptionsOriginal, &sgtOptions.tMusicOptionsOriginal);

	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), UIMSG_PAINT, 0, 0);

	UIHideOptionsDialog();
	UIRestoreLastMenu();

	// CHB 2007.10.02
	sCleanupState();
	e_UpdateGamma();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIOptionsDialogCancel( void )
{
	UIOptionsDialogCancel(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), 0, 1, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsControlsOnPaint( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);

	int nCount = NUM_KEY_COMMANDS;
	int nKey = 0;
	int nRemappableKeys = 0;
	int nScrollVal = 0;
	UI_COMPONENT *pTemp = UIComponentIterateChildren(component, NULL, UITYPE_SCROLLBAR, FALSE);
	if (pTemp)
	{
		UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pTemp);
		nScrollVal = (int)pScrollBar->m_ScrollPos.m_fY;
	}

	UI_COMPONENT *pCurrentPanel = UIComponentIterateChildren(component, NULL, UITYPE_PANEL, FALSE);
	while (nKey < nCount && pCurrentPanel)
	{
		KEY_COMMAND key = InputGetKeyCommand(nKey);
		if (key.bRemappable &&
			(key.eGame == APP_GAME_ALL || key.eGame == AppGameGet()))
		{
			nRemappableKeys++;
			if (nRemappableKeys > nScrollVal)
			{
				UI_COMPONENT * label = UIComponentFindChildByName(pCurrentPanel, "CmdName");
				ASSERT_BREAK(label);
				UI_LABELEX *pCmdLabel = UICastToLabel(label);
				UILabelSetText(pCmdLabel, StringTableGetStringByIndex(key.nStringIndex));

				const KEY_COMMAND_SAVE &tKeySetting = sgtOptions.tControlOptions.pKeyAssignments[nKey];

				for (int i=0; i < NUM_KEYS_PER_COMMAND; i++)
				{
					WCHAR szKeyName[256];
					if (!InputGetKeyNameW(tKeySetting.KeyAssign[i].nKey, tKeySetting.KeyAssign[i].nModifierKey, szKeyName, 256))
					{
						szKeyName[0] = 0;
					}

					char szBuf[256];
					PStrPrintf(szBuf, 256, "KeyBtn%d", i+1);	
					UI_COMPONENT *pKeyLabel = UIComponentFindChildByName(pCurrentPanel, szBuf);
					if (pKeyLabel)
					{
						pKeyLabel->m_dwParam = MAKE_PARAM(i, nKey);
						UILabelSetText(pKeyLabel, szKeyName);
					}
				}

				pCurrentPanel = UIComponentIterateChildren(component, pCurrentPanel, UITYPE_PANEL, FALSE);
			}
		}
		nKey++;
	}


	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionControlsScrollBarOnScroll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIScrollBarOnScroll(component, msg, wParam, lParam);

	UIOptionsControlsOnPaint(component->m_pParent, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISoundOptionControlsScrollBarOnScroll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIScrollBarOnScroll(component, msg, wParam, lParam);

	UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	sgtOptions.tSoundOptions.fSoundVolume = UIScrollBarGetValue(pDialog, "sound volume slider", TRUE);
	sgtOptions.tSoundOptions.fUIVolume = UIScrollBarGetValue(pDialog, "ui volume slider", TRUE);
	sgtOptions.tMusicOptions.fMusicVolume = UIScrollBarGetValue(pDialog, "music volume slider", TRUE);
	sgtOptions.tMusicOptions.bMuteMusic = !!UIButtonGetDown(pDialog, "music mute btn");
	sgtOptions.tSoundOptions.bMuteSound = !!UIButtonGetDown(pDialog, "sound mute btn");

	c_SoundApplySettings(&sgtOptions.tSoundOptions, &sgtOptions.tMusicOptions);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOptionsControlsOnPostActivate( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	int nCount = NUM_KEY_COMMANDS;
	int nKey = 0;
	int nRemappableKeys = 0;

	while (nKey < nCount)
	{
		KEY_COMMAND key = InputGetKeyCommand(nKey);
		if (key.bRemappable &&
			(key.eGame == APP_GAME_ALL || key.eGame == AppGameGet()))
		{
			nRemappableKeys++;
		}
		nKey++;
	}

	UI_COMPONENT *pCurrentPanel = UIComponentIterateChildren(component, NULL, UITYPE_PANEL, FALSE);
	while (pCurrentPanel)
	{
		nRemappableKeys--;
		pCurrentPanel = UIComponentIterateChildren(component, pCurrentPanel, UITYPE_PANEL, FALSE);
	}

	UI_COMPONENT *pTemp = UIComponentIterateChildren(component, NULL, UITYPE_SCROLLBAR, FALSE);
	if (pTemp)
	{
		UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pTemp);

		if (nRemappableKeys > 0)
		{
			pScrollBar->m_fMax = (float)nRemappableKeys;
			UIComponentSetActive(pScrollBar, TRUE);
		}
		else
		{
			UIComponentSetVisible(pScrollBar, FALSE);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct KeyRemapConflictCallbackData
{
	KEY_ASSIGNMENT *pKeyAssignToSet;
	KEY_ASSIGNMENT *pKeyAssignConflicting;
};

KeyRemapConflictCallbackData gtKeyRemapCallbackData;

void sKeyRemapConflictCallbackOk(
	void *pCallbackData, 
	DWORD dwCallbackData)
{
	ASSERT_RETURN(pCallbackData);
	KeyRemapConflictCallbackData *pConflictData = (KeyRemapConflictCallbackData *)pCallbackData;

	*pConflictData->pKeyAssignToSet = *pConflictData->pKeyAssignConflicting;
	pConflictData->pKeyAssignConflicting->nKey = 0;
	pConflictData->pKeyAssignConflicting->nModifierKey = 0;

	InputEndKeyRemap();
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), UIMSG_PAINT, 0, 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sKeyRemapConflictCallbackCancel(
	void *, 
	DWORD )
{
	InputEndKeyRemap();
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIOptionsRemapKey(
	int nKeyCommandBeingRemapped,
	int nKeyAssignBeingRemapped, 
	int nKey,
	int nModifierKey)
{
	ASSERT_RETURN(nKeyCommandBeingRemapped >= 0 && nKeyCommandBeingRemapped < NUM_KEY_COMMANDS);
	ASSERT_RETURN(nKeyAssignBeingRemapped >= 0  && nKeyAssignBeingRemapped < NUM_KEYS_PER_COMMAND);

	KEY_COMMAND tKeyToRemap = InputGetKeyCommand(nKeyCommandBeingRemapped);
	if (tKeyToRemap.bCheckConflicts)
	{
		for (int i=0; i < NUM_KEY_COMMANDS; i++)
		{
			const KEY_COMMAND& tKeyCheck = InputGetKeyCommand(i); //sgtOptions.tControlOptions.pKeyAssignments[i];
			if (tKeyCheck.bCheckConflicts &&
				(tKeyCheck.eGame == AppGameGet() || tKeyCheck.eGame == APP_GAME_ALL))
			{
				for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
				{
					if (sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nKey == nKey &&
						sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j].nModifierKey == nModifierKey)
					{
						if (tKeyCheck.bCheckConflicts &&
							!tKeyCheck.bRemappable)
						{
							UIShowGenericDialog(GlobalStringGet(GS_KEY_REMAP_CONFLICT_HEADER), GlobalStringGet(GS_KEY_REMAP_CONFLICT_UNAVAILABLE) , 0, 0, 0, AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);
						}
						else
						{
							WCHAR szBuf[512];
							PStrPrintf(szBuf, 512, GlobalStringGet(GS_KEY_REMAP_CONFLICT_MESSAGE), StringTableGetStringByIndex(tKeyCheck.nStringIndex));

							DIALOG_CALLBACK tOkCallback;
							DIALOG_CALLBACK tCancelCallback;

							DialogCallbackInit( tOkCallback );
							DialogCallbackInit( tCancelCallback );
							tOkCallback.pfnCallback = sKeyRemapConflictCallbackOk;
							gtKeyRemapCallbackData.pKeyAssignToSet = &(sgtOptions.tControlOptions.pKeyAssignments[nKeyCommandBeingRemapped].KeyAssign[nKeyAssignBeingRemapped]);
							gtKeyRemapCallbackData.pKeyAssignConflicting = &(sgtOptions.tControlOptions.pKeyAssignments[i].KeyAssign[j]);
							tOkCallback.pCallbackData = &gtKeyRemapCallbackData;
							tCancelCallback.pfnCallback = sKeyRemapConflictCallbackCancel;
							UIShowGenericDialog(GlobalStringGet(GS_KEY_REMAP_CONFLICT_HEADER), szBuf, TRUE, &tOkCallback, &tCancelCallback , AppIsTugboat() ? GDF_OVERRIDE_EVENTS : 0);
						}
						return;
					}
				}
			}
		}
	}

	sgtOptions.tControlOptions.pKeyAssignments[nKeyCommandBeingRemapped].KeyAssign[nKeyAssignBeingRemapped].nKey = nKey;
	sgtOptions.tControlOptions.pKeyAssignments[nKeyCommandBeingRemapped].KeyAssign[nKeyAssignBeingRemapped].nModifierKey = nModifierKey;
	InputEndKeyRemap();

	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_OPTIONSDIALOG), UIMSG_PAINT, 0, 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIKeyRemapOnLButtonDown( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (UIComponentGetActive(component) && 
		UIComponentCheckBounds(component) &&
		!InputIsRemappingKey())
	{
		int nKeyCommand = HIWORD(component->m_dwParam);
		int nKeySlot = LOWORD(component->m_dwParam);
		UILabelSetText(component, GlobalStringGet( GS_UI_KEY_REMAP_INSTRUCTIONS ) );

		// Get a new key
		InputBeginKeyRemap(nKeyCommand, nKeySlot);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIXfireLogin( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!UIComponentGetActive(component))	
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ((msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP) &&
		wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!c_VoiceChatIsLoggedIn())
	{
		// *** xfire login options
		const WCHAR *pszXfireUsernameW = UILabelGetTextByEnum(UICOMP_XFIRE_USERNAME);
		const WCHAR *pszXfirePasswordW = UILabelGetTextByEnum(UICOMP_XFIRE_PASSWORD);
		PStrCvt(sgtOptions.tSoundOptions.pszXfireUsername, pszXfireUsernameW, arrsize(sgtOptions.tSoundOptions.pszXfireUsername));
		PStrCvt(sgtOptions.tSoundOptions.pszXfirePassword, pszXfirePasswordW, arrsize(sgtOptions.tSoundOptions.pszXfirePassword));
		
		// lowercase username
		WCHAR pszLowerXfireUsernameW[256];
		PStrCvt(pszLowerXfireUsernameW, pszXfireUsernameW, arrsize(pszLowerXfireUsernameW));
		PStrLower(pszLowerXfireUsernameW, arrsize(pszLowerXfireUsernameW));
		
		c_VoiceChatLogin(pszLowerXfireUsernameW,pszXfirePasswordW);
	}
	else
	{
		c_VoiceChatLogout();
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIOptionsXfireSetStatus(BOOL bConnected, BOOL bLoginError)
{
	if (bLoginError)
	{
		const WCHAR * szText = GlobalStringGet(GS_XFIRE_LOGIN_FAILED);
		UILabelSetTextByEnum(UICOMP_XFIRE_STATUS, szText);

		const WCHAR *szButtonText = GlobalStringGet(GS_XFIRE_LOGIN);
		UILabelSetTextByEnum(UICOMP_XFIRE_LOGIN_BUTTON_TEXT, szButtonText);
	}
	else if (bConnected)
	{
		const WCHAR *szName = c_VoiceChatGetLoggedUsername();

		const WCHAR *szFormat = GlobalStringGet(GS_XFIRE_ONLINE);
		WCHAR szText[256];
		PStrPrintf(szText, arrsize(szText), szFormat, szName);
		UILabelSetTextByEnum(UICOMP_XFIRE_STATUS, szText);

		const WCHAR *szButtonText = GlobalStringGet(GS_XFIRE_LOGOUT);
		UILabelSetTextByEnum(UICOMP_XFIRE_LOGIN_BUTTON_TEXT, szButtonText);
	}
	else
	{
		const WCHAR *szName = UILabelGetTextByEnum(UICOMP_XFIRE_USERNAME);

		const WCHAR *szFormat = GlobalStringGet(GS_XFIRE_OFFLINE);
		WCHAR szText[256];
		PStrPrintf(szText, arrsize(szText), szFormat, szName);
		UILabelSetTextByEnum(UICOMP_XFIRE_STATUS, szText);	

		const WCHAR *szButtonText = GlobalStringGet(GS_XFIRE_LOGIN);
		UILabelSetTextByEnum(UICOMP_XFIRE_LOGIN_BUTTON_TEXT, szButtonText);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesOptions(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name							function pointer
		{ "UIOptionsControlsOnPaint",				UIOptionsControlsOnPaint },
		{ "UIOptionsDialogAccept",					UIOptionsDialogAccept },
		{ "UIOptionsDialogCancel",					UIOptionsDialogCancel },
		{ "UIOptionsGammaAdjust",					UIOptionsGammaAdjust },
		{ "UIOptionsControlsOnPostActivate",		UIOptionsControlsOnPostActivate },
		{ "UIOptionsDialogOnPostActivate",			UIOptionsDialogOnPostActivate },
		{ "UIOptionsDialogOnPostInactivate",		UIOptionsDialogOnPostInactivate },
		{ "UIOptionControlsScrollBarOnScroll",		UIOptionControlsScrollBarOnScroll },
		{ "UISoundOptionControlsScrollBarOnScroll",	UISoundOptionControlsScrollBarOnScroll },
		{ "UISpeakerConfigOnPostCreate",			UISpeakerConfigOnPostCreate },
		{ "UISpeakerConfigOnSelect",				UISpeakerConfigOnSelect },
//		{ "UIScreenResolutionComboOnPostCreate",	UIScreenResolutionComboOnPostCreate },	// CHB 2007.01.25 - Not currently used.
		{ "UIScreenResolutionComboOnSelect",		UIScreenResolutionComboOnSelect },
//		{ "UIScreenRefreshComboOnPostCreate",		UIScreenRefreshComboOnPostCreate },		// CHB 2007.01.25 - Not currently used.
//		{ "UIScreenRefreshComboOnSelect",			UIScreenRefreshComboOnSelect },			// CHB 2007.01.25 - Not currently used.
		{ "UIWindowedButtonClicked",				UIWindowedButtonClicked },				// CHB 2007.02.01
//		{ "UIScreenAspectRatioComboOnPostCreate",	UIScreenAspectRatioComboOnPostCreate },	// CHB 2007.02.01
		{ "UIScreenAspectRatioComboOnSelect",		UIScreenAspectRatioComboOnSelect },		// CHB 2007.02.01
		{ "UIGraphicOptionsDialogOnPostActivate",	UIGraphicOptionsDialogOnPostActivate },	// CHB 2007.05.24
		{ "UIKeyRemapOnLButtonDown",				UIKeyRemapOnLButtonDown },
		{ "UIXfireLogin",							UIXfireLogin },
		{ "UIScaleOnPostCreate",					UIScaleOnPostCreate },
		{ "UIScaleOnSelect",						UIScaleOnSelect },
		{ "UIGammaReferencePaint",					UIGammaReferencePaint },
		{ "UIOptionsButtonCheckNeedRestart",		UIOptionsButtonCheckNeedRestart },
		{ "UIOptionsControlsDefault",				UIOptionsControlsDefault },
		{ "UIOptionsGraphicsDefaults",				UIOptionsGraphicsDefaults },			// CHB 2007.10.22
		{ "UIAsModifierButtonOnClick",				UIAsModifierButtonOnClick },
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}


#endif
