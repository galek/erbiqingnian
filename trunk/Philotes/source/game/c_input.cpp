//----------------------------------------------------------------------------
// c_input.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "c_input.h"
#include "c_townportal.h"
#include "clients.h"
#include "c_message.h"
#include "c_camera.h"
#include "unit_priv.h"
#include "player.h"
#include "console.h"
#include "console_priv.h"
#include "appcommontimer.h"
#include "npc.h"
#include "skills.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_debug.h"
#include "uix_components.h"
#include "uix_quests.h"
#include "uix_social.h"
#include "uix_chat.h"
#include "uix_email.h"
#include "uix_auction.h"
#include "items.h"
#include "quests.h"
#include "inventory.h"
#include "markup.h"
#include "dxC\\dxC_input.h"
#include "e_settings.h"
#include "e_screenshot.h"
#include "unittag.h"
#include "skills_shift.h"
#include "dialog.h"
#include "globalindex.h"
#include "objects.h"
#include "npc.h"
#include "uix_components_hellgate.h"
#include "c_quests.h"
#include "camera_priv.h"
#include "movement.h"
#include "states.h"
#include "c_ui.h"
#include "c_movieplayer.h"
#include "teams.h"
#include "c_itemupgrade.h"
#include "quest_tutorial.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "weaponconfig.h"
#include "uix_options.h"
#include "windowsmessages.h"
#include "gameoptions.h"
#include "keyconfig.h"
#include "uix_priv.h"
#include "e_optionstate.h"
#include "common/resource.h"
#include "demolevel.h"
#include "controloptions.h"
#include "consolecmd.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define KEY_CONFIG_FILE		"KeyConfig.xml"

#define MOUSE_BUFFER_SIZE	32

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
KEY_COMMAND g_pKeyAssignments[] = 
{
	#include "key_command_priv.h"
};


struct CINPUT
{
	SPDIRECTINPUT8			m_pDirectInput;
	SPDIRECTINPUTDEVICE8	m_pMouse;
	HANDLE					m_hMouseEvent;

	BOOL					m_bJustActivated;
	BOOL					m_bMouseAcquired;
	BOOL					m_bInvertMouse;
	BOOL					m_bMouseLook;

	BOOL					m_bAdvancedCamera;
	BOOL					m_bLockedPitch;
	BOOL					m_bFollowCamera;

	float					m_fMouseX;
	float					m_fMouseY;
	DWORD					m_dwMouseButtonState;
	int						m_nScreenCenterX;
	int						m_nScreenCenterY;

	ULONG					m_ulRawButtons;

	float					m_fMouseSensitivity;
	float					m_fMouseLookSensitivity;

	int						m_nKeyCommandBeingRemapped;
	int						m_nKeyAssignBeingRemapped;

	BOOL					m_bVanityDown;
	BOOL					m_bLeftSkillActive;
	UNITID					m_idOldTarget;

	TIME					m_tiBlockKeyInputUntil;
	TIME					m_tiBlockMouseBtnUntil;

#if ISVERSION(CHEATS_ENABLED)
	unsigned int			m_nCheatCodeMatch;
	unsigned int			m_nCheatCodeLen;
#endif
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
CINPUT gInput;

#if ISVERSION(CHEATS_ENABLED)
static const char * CHEAT_CODE = "YSHIPPY";
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL InputHandleKey(
	GAME * game,
	UINT uMsg,
	DWORD wKey,
	DWORD lParam);

static void sInitKeyCmd(
	void);

static BOOL sbUsingDirectInput = FALSE;
inline BOOL c_UsingDirectInput()
{
	return sbUsingDirectInput;
}

BOOL c_SetUsingDirectInput(
	BOOL bUse)
{
	if (bUse != sbUsingDirectInput)
	{
		c_InputClose();
		sbUsingDirectInput = bUse;
		c_InputInit(AppCommonGetHWnd(), AppGetHInstance());
	}
	return sbUsingDirectInput;
}

static void sAbortVanity(
	GAME * game)
{
	if (c_CameraGetViewType() == VIEW_VANITY)
	{
		c_CameraRestoreViewType();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetAdvancedCamera(
	void)
{
	return gInput.m_bAdvancedCamera;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputSetAdvancedCamera(
	BOOL value)
{
	gInput.m_bAdvancedCamera = value;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetLockedPitch(
	void)
{
	return gInput.m_bLockedPitch;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputSetLockedPitch(
	BOOL value)
{
	gInput.m_bLockedPitch = value;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetFollowCamera(
	void)
{
	return gInput.m_bFollowCamera;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputSetFollowCamera(
	BOOL value)
{
	gInput.m_bFollowCamera = value;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetMouseInverted(
	void)
{
	return gInput.m_bInvertMouse;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputSetMouseInverted(
	BOOL value)
{
	gInput.m_bInvertMouse = value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL InputGetMouseAcquired(
	void)
{
	return gInput.m_bMouseAcquired;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL InputGetMouseLook(
	void)
{
	return gInput.m_bMouseLook;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void InputSetMouseLook(
	BOOL value)
{
	gInput.m_bMouseLook = value;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_COMMAND & InputGetKeyCommand(
	int nCommand)
{
	ASSERT(nCommand >=0 && nCommand < NUM_KEY_COMMANDS);

	return g_pKeyAssignments[nCommand];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const KEY_COMMAND * InputGetKeyCommand(
	const char *szCommandName)
{
	for (int nCommand = 0; nCommand < NUM_KEY_COMMANDS; nCommand++)
	{
		if (g_pKeyAssignments[nCommand].szCommandStr && PStrICmp(szCommandName, g_pKeyAssignments[nCommand].szCommandStr) == 0)
		{
			return &(g_pKeyAssignments[nCommand]);
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputIsCommandKeyDown(
	int nCommand)
{
	ASSERT(nCommand >=0 && nCommand < NUM_KEY_COMMANDS);

	for (unsigned int ii = 0; ii < NUM_KEYS_PER_COMMAND; ii++)
	{
		SHORT nKeyState = GetAsyncKeyState(g_pKeyAssignments[nCommand].KeyAssign[ii].nKey);
		SHORT nModKeyState = GetAsyncKeyState(g_pKeyAssignments[nCommand].KeyAssign[ii].nModifierKey);
//		trace ("AsyncKeyState %d = %u\n", nCommand, nKeyState);
		if (( nKeyState & 0x8000) &&
	  	    (g_pKeyAssignments[nCommand].KeyAssign[ii].nModifierKey == 0 || 
                 nModKeyState & 0x8000))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputGetMousePosition(
	float * x,
	float * y)
{
	*x = gInput.m_fMouseX;
	*y = gInput.m_fMouseY;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetMouseButtonDown(
	void)
{
	return gInput.m_dwMouseButtonState != 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int GetMouseVKButton(int nButton)
{
	ASSERT_RETINVALID(nButton >= 0 && nButton < MOUSE_BUTTON_COUNT);
	switch(nButton)
	{
	case MOUSE_BUTTON_LEFT:		return VK_LBUTTON;
	case MOUSE_BUTTON_RIGHT:	return VK_RBUTTON;
	case MOUSE_BUTTON_MIDDLE:	return VK_MBUTTON;
	case MOUSE_BUTTON_X1:		return VK_XBUTTON1;
	case MOUSE_BUTTON_X2:		return VK_XBUTTON2;
	}

	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetMouseButtonDown(
	int nButton)
{
	//if (c_UsingDirectInput())
	{
		ASSERT_RETFALSE(nButton >= 0 && nButton < MOUSE_BUTTON_COUNT);
		return TESTBIT(&gInput.m_dwMouseButtonState, nButton);
	}
	//else
	//{
	//	return GetAsyncKeyState(GetMouseVKButton(nButton));
	//}
}

static VECTOR sGetPlayerClickLocation( GAME * game )
{
	VECTOR vPlayClickLocation( 0 );
	UNIT *player = GameGetControlUnit( game );				
	vPlayClickLocation = UnitGetPosition( player );
	VECTOR vStart, vDirection;
	UI_TB_MouseToScene( game, &vStart, &vDirection );
	if( vDirection.fZ > -.02f )
	{
		vDirection.fZ = -.02f;
		VectorNormalize( vDirection );
	}
	float fLength = ( vPlayClickLocation.z - vStart.fZ ) / vDirection.fZ;
	vPlayClickLocation.fX = vStart.fX + ( vDirection.fX * fLength );
	vPlayClickLocation.fY = vStart.fY + ( vDirection.fY * fLength );
	vPlayClickLocation.fZ = vPlayClickLocation.fZ;	
	return vPlayClickLocation;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStartSkillRequestByID(
	GAME * game, 
	int nSkill )
{
	ASSERT_RETFALSE(IS_CLIENT(game));

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitGetRoom(unit))
	{
		return FALSE;
	}
	

	if(nSkill != INVALID_ID)
	{
		VECTOR vPlayClickLocation( 0 );
		
		if( AppIsTugboat() )
		{			
			const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );
			if( SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAYER_STOP_MOVING ) )
			{
				c_PlayerStopPath( game );

			}

			c_PlayerClearMovement(game, PLAYER_AUTORUN);
			vPlayClickLocation = sGetPlayerClickLocation( game )	;
			
		}
		
		BOOL bSuccess = c_SkillControlUnitRequestStartSkill( game, nSkill, NULL, vPlayClickLocation );

		return bSuccess;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sToggleAggression( GAME * game, 
							   UNIT * target )
{
	ASSERT_RETFALSE(IS_CLIENT(game));

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitGetRoom(unit))
	{
		return FALSE;
	}
	if( !target || !UnitIsA( target, UNITTYPE_PLAYER ) )
	{
		return FALSE;
	}
	
	if( target && ( UnitGetPartyId( target ) == INVALID_ID ||
		 UnitGetPartyId( target ) != UnitGetPartyId( unit ) ) )
	{
		MSG_CCMD_AGGRESSION_TOGGLE msg;
		msg.idTarget = UnitGetId(target);
		c_SendMessage(CCMD_AGGRESSION_TOGGLE, &msg);

		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStartSkillRequest(
	GAME * game, 
	int nSkillStat)
{
	ASSERT_RETFALSE(IS_CLIENT(game));

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitGetRoom(unit))
	{
		return FALSE;
	}
	int skill = UnitGetStat(unit, nSkillStat);
	return sStartSkillRequestByID( game, skill );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sEndSkillRequestByID(
	GAME * game, 
	int nSkill)
{
	ASSERT_RETURN(IS_CLIENT(game));

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}
	
	BOOL bSkillCurrentlyActive = FALSE;
	if( SkillGetLevel( unit, nSkill) > 0 )
	{
		UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
		UnitGetWeapons( unit, nSkill, pWeapons, FALSE );


		for ( int i = 0; i < MAX_WEAPON_LOCATIONS_PER_SKILL; i++ )
		{
			if ( !pWeapons[ i ] )
				continue;

			int nWeaponSkill = ItemGetPrimarySkill( pWeapons[ i ] );
			if ( nWeaponSkill == INVALID_ID )
				continue;
			int nTicksToHold = SkillNeedsToHold( unit, pWeapons[ i ], nWeaponSkill );
			if( nTicksToHold > 0 )
			{
				bSkillCurrentlyActive = TRUE;
				break;
			}

		}
		if( !bSkillCurrentlyActive )
		{
			int nTicksToHold = SkillNeedsToHold( unit, NULL, nSkill, TRUE );
			if( nTicksToHold > 0 )
			{
				bSkillCurrentlyActive = TRUE;
			}
		}
		
	}	

	SkillStopRequest(unit, nSkill, TRUE, !bSkillCurrentlyActive);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sEndSkillRequest(
	GAME * game, 
	int nSkillStat)
{
	ASSERT_RETURN(IS_CLIENT(game));

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}
	int skill = UnitGetStat(unit, nSkillStat);
	sEndSkillRequestByID( game, skill );
}


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInputInternalSetMousePosition(
	float x,
	float y)
{
	//float fNewX = PIN(x, 0.0f, AppCommonGetWindowWidth() - 1.0f);
	//float fNewY = PIN(y, 0.0f, AppCommonGetWindowHeight() - 1.0f);
	if (x < 0.0f ||
		y < 0.0f ||
		x >= AppCommonGetWindowWidth() ||
		y >= AppCommonGetWindowHeight() )
	{
		return FALSE;
	}

	if (x != gInput.m_fMouseX ||
		y != gInput.m_fMouseY)				// KCK: y was "x". Seems like an error to me.
	{
		gInput.m_fMouseX = x;
		gInput.m_fMouseY = y;
		UICursorSetPosition(gInput.m_fMouseX, gInput.m_fMouseY);
	}

	return TRUE;
}

void InputSetMousePosition(
	float x,
	float y)
{
	POINT pt;
	pt.x = (LONG)x;
	pt.y = (LONG)y;
	::ClientToScreen(AppCommonGetHWnd(), &pt);

	::SetCursorPos(pt.x, pt.y);
	sInputInternalSetMousePosition(x, y);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputResetMousePosition()
{
	InputSetMousePosition(AppCommonGetWindowWidth() / 2.0f, AppCommonGetWindowHeight() / 2.0f);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_InputMouseAcquire(
	GAME * game,
	BOOL bAcquire)
{
	if ( ! e_OptionState_IsInitialized() )
		return 0;

	int nCount = 0;
	if (c_UsingDirectInput())
	{
		if (!gInput.m_pMouse)
			return FALSE;

		if (bAcquire)
		{
			gInput.m_pMouse->Acquire();
		}
		else
		{
			gInput.m_pMouse->Unacquire();
		}
		gInput.m_bMouseAcquired = bAcquire;
	}
	else
	{
		HWND hwnd = AppCommonGetHWnd();
		if (gInput.m_bMouseAcquired != bAcquire)
		{
			if (bAcquire)
			{


				if (AppIsHellgate())
				{
					do		// make sure it's turned off.
					{

						nCount = ::ShowCursor(FALSE);
					} while (nCount >= 0);

				}
				if (e_IsFullscreen())
				{
					gInput.m_nScreenCenterX = (int)(AppCommonGetWindowWidth() / 2);
					gInput.m_nScreenCenterY = (int)(AppCommonGetWindowHeight() / 2);

				}
				else
				{
					RECT rectClip;
					::GetClientRect(hwnd, &rectClip); 

					POINT ptTL, ptBR;
					ptTL.x = rectClip.left;
					ptTL.y = rectClip.top;
					ptBR.x = rectClip.right;
					ptBR.y = rectClip.bottom;
					::ClientToScreen(hwnd, &ptTL);
					::ClientToScreen(hwnd, &ptBR);
					rectClip.left = ptTL.x;
					rectClip.top = ptTL.y;
					rectClip.right = ptBR.x;
					rectClip.bottom = ptBR.y;

					gInput.m_nScreenCenterX = ptTL.x + ((ptBR.x - ptTL.x) / 2);
					gInput.m_nScreenCenterY = ptTL.y + ((ptBR.y - ptTL.y) / 2);

					//trace("Clipping cursor to (%d, %d, %d, %d)\n", rectClip.left, rectClip.top, rectClip.right, rectClip.bottom);
					//ConsoleString("Clipping cursor to (%d, %d, %d, %d)\n", rectClip.left, rectClip.top, rectClip.right, rectClip.bottom);
				}

			}
			else
			{
				// CHB 2007.08.23 - so that mouse pointer is visible over game window
				if( AppIsHellgate() )
				{
					do
					{
						nCount = ::ShowCursor(true);
					} while (nCount < 0);
					while (nCount > 0)
					{
						nCount = ::ShowCursor(false);
					}
				}


			}
			if( AppIsTugboat() )
			{
				UIUpdateHardwareCursor();
			}
			//trace("Mouse Acquire count = %d\n", nCount);
			gInput.m_bMouseAcquired = bAcquire;
		}
	}

	if (bAcquire && game && IS_CLIENT(game))
	{
		// clear all keys
		c_PlayerClearAllMovement(game);
		gInput.m_bJustActivated = TRUE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_InputInit(
	HWND hWnd,
	HINSTANCE hInstance)
{
	structclear(gInput);

	InputEndKeyRemap();
#if ISVERSION(CHEATS_ENABLED)
	gInput.m_nCheatCodeLen = PStrLen(CHEAT_CODE);
#endif

	KeyConfigInit();

	// set up the mouse
	InputSetMouseLook(TRUE);
	InputSetMousePosition(AppCommonGetWindowWidth() / 2.0f, AppCommonGetWindowHeight() / 2.0f);

	if (c_UsingDirectInput())
	{
		ASSERT_RETFALSE(SUCCEEDED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&gInput.m_pDirectInput, NULL)));

		gInput.m_dwMouseButtonState = 0;

		ASSERT_RETFALSE(SUCCEEDED(gInput.m_pDirectInput->CreateDevice(GUID_SysMouse, &gInput.m_pMouse, NULL)));

		ASSERT_RETFALSE(SUCCEEDED(gInput.m_pMouse->SetDataFormat(&c_dfDIMouse)));

		ASSERT_RETFALSE(SUCCEEDED(gInput.m_pMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)));

		gInput.m_hMouseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		ASSERT_RETFALSE(gInput.m_hMouseEvent);

		ASSERT_RETFALSE(SUCCEEDED(gInput.m_pMouse->SetEventNotification(gInput.m_hMouseEvent)));

		// tell direct input how much to buffer the input from the mouse
		DIPROPDWORD dipdw;
		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj = 0;
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = MOUSE_BUFFER_SIZE;

		ASSERT_RETFALSE(SUCCEEDED(gInput.m_pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)));

		ASSERT_RETFALSE(c_InputMouseAcquire(NULL, TRUE));
	}
	else
	{
		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
		Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
		Rid[0].dwFlags = 0;	//RIDEV_INPUTSINK;   
		Rid[0].hwndTarget = AppCommonGetHWnd();
		RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));

		ASSERT_RETFALSE(c_InputMouseAcquire(NULL, TRUE));

	}

	const GLOBAL_DEFINITION * globals = DefinitionGetGlobal();
	ASSERT(globals);
	if (globals && (globals->dwFlags & GLOBAL_FLAG_INVERTMOUSE))
	{
		c_ToggleInvertMouse();
	}

	gInput.m_fMouseSensitivity = 1.0f;
	gInput.m_fMouseLookSensitivity = 1.0f;

	if( AppIsTugboat() )
	{
		const CONFIG_DEFINITION * config = DefinitionGetConfig();
		if (config)
		{
			gInput.m_fMouseSensitivity = config->fMouseSensitivity;
		}
		if ( gInput.m_fMouseSensitivity == 0 )
		{
			gInput.m_fMouseSensitivity = 1.5f;
		}
	}

	sInitKeyCmd();

	gInput.m_tiBlockKeyInputUntil = 0;
	gInput.m_tiBlockMouseBtnUntil = 0;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputSetMouseSensitivity(
	float fSensitivity)
{
	gInput.m_fMouseSensitivity = PIN(fSensitivity, MOUSE_CURSOR_SENSITIVITY_MIN, MOUSE_CURSOR_SENSITIVITY_MAX);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputSetMouseLookSensitivity(
	float fSensitivity)
{
	gInput.m_fMouseLookSensitivity = PIN(fSensitivity, MOUSE_LOOK_SENSITIVITY_MIN, MOUSE_LOOK_SENSITIVITY_MAX);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_InputClose(
	void)
{
	KeyConfigFree();

	c_InputMouseAcquire(NULL, FALSE);

	if (gInput.m_pDirectInput)
	{
		ASSERTX_RETURN(c_UsingDirectInput(), "This shouldn't get set if we're not using DirectInput");
		gInput.m_pDirectInput = NULL;
	}

	if (!c_UsingDirectInput())
	{
		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
		Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
		Rid[0].dwFlags = RIDEV_REMOVE;   
		Rid[0].hwndTarget = AppCommonGetHWnd();
		RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RestartKey(
	GAME * game)
{
	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}
	if (!IsUnitDeadOrDying(unit))
	{
		return;
	}

	if (AppIsHellgate())
	{
		return;
	}
		
	c_PlayerSendRespawn( PLAYER_RESPAWN_INVALID );
	return;			  
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ActionKey(
	GAME * game)
{
	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}
	if (IsUnitDeadOrDying(unit))
	{
		if (AppIsHellgate() == FALSE)
		{
			c_PlayerSendRespawn( PLAYER_RESPAWN_INVALID );	
		}
		return;			  
	}
	UNIT * selection = (AppIsHellgate())?UIGetTargetUnit():UIGetClickedTargetUnit();
	if (!selection)
	{
		return;
	}
	if (UnitTestFlag(unit, UNITFLAG_CANPICKUPSTUFF) &&
		UnitTestFlag(selection, UNITFLAG_CANBEPICKEDUP) &&
		VectorDistanceSquared(unit->vPosition, selection->vPosition) < PICKUP_RADIUS_SQ)
	{
		c_ItemTryPickup( game, UnitGetId( selection ) );

		return;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ToggleInvertMouse(
	void)
{
	InputSetMouseInverted(!InputGetMouseInverted());
}
//Tugboat
static BOOL sgbLeftSkillActive = FALSE;
static BOOL sgbRightSkillActive = FALSE;
static BOOL sgbSpellHotkeyDown = FALSE;
static BOOL sgbPathingOnClick = FALSE;
static BOOL sgbInteracted = FALSE;
static BOOL sgbStartedLeftSkill = FALSE;
static BOOL sgbStartedRightSkill = FALSE;
static BOOL sgbJustActivated = FALSE;
static BOOL sgbPerformedInteraction = FALSE;
static int sgnLastLeftSkill = INVALID_ID;
static int sgnLastRightSkill = INVALID_ID;
static int gPreOrbitCursorPosX = -1;
static int gPreOrbitCursorPosY = -1;
static float sgfFollowCameraTimer = 0;

void InputSetInteracted(
	void )
{
	sgbPerformedInteraction = TRUE;
}

static BOOL sInRTS()
{
	if (AppIsHellgate())
	{
		GAME *pGame = AppGetCltGame();
		if (pGame)
		{
			UNIT *pControlUnit = GameGetControlUnit(pGame);
			if (pControlUnit)
			{
				if (UnitHasState(pGame, pControlUnit, STATE_QUEST_RTS))
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

static LRESULT sHandleMouseButtonHellgate(
	GAME* game,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if(AppGetState() == APP_STATE_PLAYMOVIELIST)
	{
		if(uMsg == WM_XBUTTONDOWN)
		{
			c_MoviePlayerSkipToEnd();
		}
	}

	return TRUE;

	//if (UICursorGetActive() || ( AppIsTugboat() && UIGetShowItemNamesState() ) ) 
	//{
	//	LPARAM lParam = MAKE_PARAM((int)gInput.m_fMouseX, (int)gInput.m_fMouseY);
	//	if( AppIsHellgate() )
	//	{
	//		if (!ResultIsHandled( UISendMessage((bButtonDown ? WM_LBUTTONDOWN : WM_LBUTTONUP), 0, lParam) ))
	//		{
	//			if (bButtonDown && UIGetShowItemNamesState())
	//			{
	//				if (UIUnitNameDisplayClick((int)gInput.m_fMouseX, (int)gInput.m_fMouseY))
	//				{
	//					return;
	//				}
	//				// otherwise fall through
	//			}

	//			if (!sInRTS())
	//				return;
	//		}
	//		else
	//		{
	//			return;
	//		}
	//	}
	//	else	//tugboat
	//	{
	//		if( ResultIsHandled( UISendMessage((bButtonDown ? WM_LBUTTONDOWN : WM_LBUTTONUP), 0, lParam) ) )
	//			return;
	//	}
	//}

	//if ( AppIsHellgate() && 
	//	 AppGetState() == APP_STATE_IN_GAME && 
	//	 !AppCommonIsPaused() && 
 //        game)
	//{
	//	if (bButtonDown)
	//	{
	//		UNIT *pPlayer = GameGetControlUnit( game );
	//		c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_LEFTBUTTON );
	//		UNIT *pTarget = UIGetTargetUnit();
	//		REF(pTarget);
	//		//sAbortVanity( game );

	//		UNIT *pInteractTarget = UIGetTargetUnit();
	//		if ( pInteractTarget && 
	//			(UnitCanInteractWith( pInteractTarget, pPlayer ) || 
	//			(UnitIsPlayer(pInteractTarget) && UnitIsInTown(pInteractTarget))))
	//		{
	//			if (UnitIsNPC(pInteractTarget) || UnitIsPlayer(pInteractTarget))
	//			{
	//				c_InteractRequestChoices( pInteractTarget );
	//			}
	//			else
	//			{
	//				c_PlayerInteract( game, pInteractTarget );
	//			}
	//		}
	//		//trace( "** FIRE GUN **\n ");
	//		else if ( sStartSkillRequest(game, STATS_SKILL_LEFT) )
	//		{
	//			gInput.m_bLeftSkillActive = TRUE;
	//			UNIT * target = UIGetTargetUnit();
	//			if ( target )
	//			{
	//				gInput.m_idOldTarget = UnitGetId( target );
	//			}
	//		}
	//	}
	//	else
	//	{
	//		//trace( "** FIRE END **\n ");
	//		sEndSkillRequest(game, STATS_SKILL_LEFT);
	//		gInput.m_bLeftSkillActive = FALSE;
	//		gInput.m_idOldTarget = INVALID_ID;
	//	}
	//}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInputHandleLeftButton(
	GAME* game,
	BOOL bButtonDown)
{
	if (AppCommonGetAbsTime() < gInput.m_tiBlockMouseBtnUntil)
	{
		return;
	}

	if(AppIsHellgate() && AppGetState() == APP_STATE_PLAYMOVIELIST)
	{
		if(bButtonDown)
		{
			c_MoviePlayerSkipToEnd();
		}
	}

	if (UICursorGetActive() || ( AppIsTugboat() && UIGetShowItemNamesState() ) ) 
	{
		LPARAM lParam = MAKE_PARAM((int)gInput.m_fMouseX, (int)gInput.m_fMouseY);
		if( AppIsHellgate() )
		{
			if (!ResultIsHandled( UISendMessage((bButtonDown ? WM_LBUTTONDOWN : WM_LBUTTONUP), 0, lParam) ))
			{
				if (bButtonDown && UIGetShowItemNamesState())
				{
					if (UIUnitNameDisplayClick((int)gInput.m_fMouseX, (int)gInput.m_fMouseY))
					{
						return;
					}
					// otherwise fall through
				}

				if (!sInRTS())
					return;
			}
			else
			{
				return;
			}
		}
		else	//tugboat
		{
			if( ResultIsHandled( UISendMessage((bButtonDown ? WM_LBUTTONDOWN : WM_LBUTTONUP), 0, lParam) ) )
				return;
		}
	}

	if ( AppIsHellgate() && 
		 AppGetState() == APP_STATE_IN_GAME && 
		 !AppCommonIsPaused() && 
         game)
	{
		if (bButtonDown)
		{
			UNIT *pPlayer = GameGetControlUnit( game );
			c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_LEFTBUTTON );
			UNIT *pTarget = UIGetTargetUnit();
			REF(pTarget);
			//sAbortVanity( game );

			UNIT *pInteractTarget = UIGetTargetUnit();
			if ( pInteractTarget && 
				(UnitCanInteractWith( pInteractTarget, pPlayer ) || 
				(UnitIsPlayer(pInteractTarget) && UnitIsInTown(pInteractTarget))))
			{
				if (UnitIsNPC(pInteractTarget) || UnitIsPlayer(pInteractTarget))
				{
					c_InteractRequestChoices( pInteractTarget );
				}
				else
				{
					c_PlayerInteract( game, pInteractTarget );
				}
			}
			//trace( "** FIRE GUN **\n ");
			else if ( sStartSkillRequest(game, STATS_SKILL_LEFT) )
			{
				gInput.m_bLeftSkillActive = TRUE;
				UNIT * target = UIGetTargetUnit();
				if ( target )
				{
					gInput.m_idOldTarget = UnitGetId( target );
				}
			}
		}
		else
		{
			//trace( "** FIRE END **\n ");
			sEndSkillRequest(game, STATS_SKILL_LEFT);
			gInput.m_bLeftSkillActive = FALSE;
			gInput.m_idOldTarget = INVALID_ID;
		}
	}
	
	else if( AppIsTugboat() &&
			 AppGetState() == APP_STATE_IN_GAME && 
			 !AppCommonIsPaused() && 
			 game )
	{

		UNIT *pUnit = GameGetControlUnit( game );
		UNIT *pTarget = UIGetTargetUnit();
		LEVEL *pLevel = UnitGetLevel( pUnit );
		if( pUnit &&  IsUnitDeadOrDying( pUnit ) )
		{
			return;
		}
		c_PlayerClearMovement(game, PLAYER_AUTORUN);
		int nSkillLeft = UnitGetStat( pUnit, STATS_SKILL_LEFT );
		
		//sAbortVanity( game );
		if( pTarget && pUnit &&
			UnitGetLevel( pTarget ) != UnitGetLevel( pUnit ) )
		{
			pTarget = NULL;
		}
		BOOL OverPanel = UI_TB_MouseOverPanel();

		if( bButtonDown )
		{
			if ( !OverPanel && InputIsCommandKeyDown( CMD_SKILL_SHIFT ) )
			{
				pTarget = NULL;
				UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, nSkillLeft );
				sgbPathingOnClick = TRUE;
			}
			if( !OverPanel && InputIsCommandKeyDown( CMD_FORCEMOVE ) )
			{
				pTarget = NULL;
				UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, nSkillLeft );
				sgbPathingOnClick = TRUE;
			}

			if ( !OverPanel && InputIsCommandKeyDown( CMD_ENGAGE_PVP ) && pTarget &&
				!LevelIsSafe( pLevel ) && PlayerIsInPVPWorld( pUnit ) )
			{
				sToggleAggression( game, pTarget );
				return;
			}


			if ( SkillIsOnWithFlag( pUnit, SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT ) )
				return;

			if ( c_CameraGetViewType() >= VIEW_3RDPERSON )
			{
				if( pTarget && UnitIsA( pTarget, UNITTYPE_PLAYER ) && !TestHostile( game, pUnit, pTarget ))
				{
					pTarget = NULL;
				}
				if ( pTarget && c_CameraGetViewType() != VIEW_VANITY)
				{

					if ( pTarget && UnitCanInteractWith( pTarget, pUnit ) ) //items now can be interactive
					{
						UISetClickedTargetUnit( UnitGetId(pTarget), TRUE, TRUE, INVALID_ID );
						float Distance = UNIT_INTERACT_INITIATE_DISTANCE_SQUARED;
						if( ObjectIsDoor( pTarget ) )
						{
							Distance *= 4;
						}
						if ( UnitsGetDistanceSquaredMinusRadiusXY( GameGetControlUnit( game ), pTarget )  < Distance )
						{
							if( ObjectIsDoor( pTarget ) )
							{
								c_PlayerStopPath( game );
							}
							sgfFollowCameraTimer = 0;
							sEndSkillRequest(game, STATS_SKILL_LEFT);
							sgbLeftSkillActive = FALSE;
							c_PlayerMouseAimLocation( game, &pTarget->vPosition );
							c_PlayerInteract( game, pTarget, UNIT_INTERACT_DEFAULT, TRUE );
							c_PlayerStopPath( game );
							sgbInteracted = TRUE;
							// done interacting
							UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );
						}
						else
						{
							sEndSkillRequest(game, STATS_SKILL_LEFT);
							sgbLeftSkillActive = FALSE;
							c_PlayerMoveToLocation( game, &pTarget->vPosition );
						} 
					}
					else if ( UnitTestFlag(pUnit, UNITFLAG_CANPICKUPSTUFF) && UnitTestFlag( pTarget, UNITFLAG_CANBEPICKEDUP ) )
					{
						UISetClickedTargetUnit( UnitGetId(pTarget), TRUE, TRUE, INVALID_ID );
						if ( UnitsGetDistanceSquaredXY( GameGetControlUnit( game ), pTarget )  < UNIT_INTERACT_INITIATE_DISTANCE_SQUARED * 2 )
						{
							sgfFollowCameraTimer = 0;
							sEndSkillRequest(game, STATS_SKILL_LEFT);
							sgbLeftSkillActive = FALSE;
							c_PlayerMouseAimLocation( game, &pTarget->vPosition ); 
							c_ActionKey( game );
							sgbInteracted = TRUE;
						}
						else
						{
							sEndSkillRequest(game, STATS_SKILL_LEFT);
							sgbLeftSkillActive = FALSE;
							c_PlayerMoveToLocation( game, &pTarget->vPosition );
						}
					}
					else if( ( ( UnitIsA( pTarget, UNITTYPE_DESTRUCTIBLE ) &&
						!IsUnitDeadOrDying(pTarget) ) ||
						( !IsUnitDeadOrDying(pTarget) &&
						UnitGetTeam( pTarget ) != INVALID_TEAM && TestHostile( game, pUnit, pTarget ) ) ) )
					{
						UISetClickedTargetUnit( UnitGetId(pTarget), TRUE, TRUE, nSkillLeft );
						// try to kick it if it is destructible
						if( UnitIsA( pTarget, UNITTYPE_DESTRUCTIBLE )  )
						{
							if( PlayerIsInAttackRange( game, pTarget, UnitGetData( pUnit )->nKickSkill, TRUE ) )
							{
								c_PlayerStopPath( game );
								c_PlayerMouseAimLocation( game, &pTarget->vPosition );

								sStartSkillRequestByID( game, UnitGetData( pUnit )->nKickSkill );
								sEndSkillRequestByID( game, UnitGetData( pUnit )->nKickSkill );
							}
							else
							{
								c_PlayerMoveToLocation( game, &pTarget->vPosition );
								sEndSkillRequest(game, STATS_SKILL_LEFT);
								sgbLeftSkillActive = FALSE;
							}
						}
						else if( PlayerIsInAttackRange( game, pTarget, nSkillLeft, TRUE ) )
						{
							c_PlayerStopPath( game );
							c_PlayerMouseAimLocation( game, &pTarget->vPosition );
							sgbStartedLeftSkill = TRUE;
							sStartSkillRequest( game, STATS_SKILL_LEFT );
							sgbInteracted = TRUE;

							sgbLeftSkillActive = TRUE;
						}
						else
						{
							c_PlayerMoveToLocation( game, &pTarget->vPosition );
							sEndSkillRequest(game, STATS_SKILL_LEFT);
							sgbLeftSkillActive = FALSE;
						}

					}
					else
					{
						UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, nSkillLeft );
						sEndSkillRequest(game, STATS_SKILL_LEFT);
						sgbLeftSkillActive = FALSE;
						c_PlayerMoveToMouse( game );
						sgbPathingOnClick = TRUE;
					}
				}
				else
				{
					UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );

					if (c_CameraGetViewType() != VIEW_VANITY)
					{
						if ( InputIsCommandKeyDown( CMD_SKILL_SHIFT ) )
						{
							UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, nSkillLeft );
							c_PlayerStopPath( game );
							c_PlayerMouseAim( game, TRUE );
							sgbStartedLeftSkill = TRUE;
							sStartSkillRequest(game, STATS_SKILL_LEFT );
							sgbInteracted = TRUE;

							sgbLeftSkillActive = TRUE;
							sgbPathingOnClick = TRUE;
						}
						else
						{
							sgbPathingOnClick = TRUE;
							sEndSkillRequest(game, STATS_SKILL_LEFT );
							sgbLeftSkillActive = FALSE;
							c_PlayerMoveToMouse( game );
						}
					}
				}

			}
		}
		else
		{
			if ( SkillIsOnWithFlag( pUnit, SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT ) ||
				 OverPanel )
			{
				sgbPerformedInteraction = FALSE;
				return;
			}
			if ( c_CameraGetViewType() >= VIEW_3RDPERSON )
			{
				if( pTarget && UnitIsA( pTarget, UNITTYPE_PLAYER ) && !TestHostile( game, pUnit, pTarget ))
				{
					pTarget = NULL;
				}
				if ( pTarget && c_CameraGetViewType() != VIEW_VANITY)
				{
				}
				else
				{
					UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );

					if (c_CameraGetViewType() != VIEW_VANITY)
					{
						if ( !InputIsCommandKeyDown( CMD_SKILL_SHIFT ) && !sgbPerformedInteraction)
						{
							// force a full path to location on key up
							c_PlayerMoveToMouse( game, TRUE );
						}
					}
				}
			}
			sgbPerformedInteraction = FALSE;

		}
		

	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInputHandleRightButtonSkill(
	GAME* game,
	BOOL bButtonDown,
	int nSkillOverride = INVALID_ID )
{
	if (AppGetState() == APP_STATE_IN_GAME && !AppCommonIsPaused() && game && bButtonDown )
	{
		UNIT *pUnit = GameGetControlUnit( game );
		if( pUnit &&  IsUnitDeadOrDying( pUnit ) )
		{
			return;
		}

		c_PlayerClearMovement(game, PLAYER_AUTORUN);

		if ( SkillIsOnWithFlag( pUnit, SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT ) )
			return;

		UNIT *pTarget = UIGetTargetUnit();
		if( c_CameraGetViewType() == VIEW_VANITY )
		{
			sAbortVanity( game );			
			return;
		}
		int nSkillRight = nSkillOverride != INVALID_ID ? nSkillOverride : ( UnitGetStat( GameGetControlUnit( game ), STATS_SKILL_RIGHT ) );
		if( nSkillRight != sgnLastRightSkill &&
			sgnLastRightSkill != INVALID_ID )
		{
			sEndSkillRequestByID(game, sgnLastRightSkill );
		}
		sgnLastRightSkill = nSkillRight;
		if( nSkillRight == INVALID_ID )
		{
			return;
		}
		if( pTarget && pUnit &&
			UnitGetLevel( pTarget ) != UnitGetLevel( pUnit ) )
		{
			pTarget = NULL;
		}
		if ( InputIsCommandKeyDown( CMD_SKILL_SHIFT ) )
		{
			pTarget = NULL;
			UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );
			sgbPathingOnClick = TRUE;
		}



		if ( c_CameraGetViewType() >= VIEW_3RDPERSON )
		{
			if ( pTarget )
			{
				
				const SKILL_DATA * pSkillData = SkillGetData( game, nSkillRight );

				BOOL bCanTarget = FALSE;
				if ( pSkillData &&
					SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) )
				{
					bCanTarget = TRUE;
				}

				if( bCanTarget && 
					( ( UnitIsA( pTarget, UNITTYPE_DESTRUCTIBLE ) &&
					!IsUnitDeadOrDying(pTarget) ) ||
					 ( !IsUnitDeadOrDying(pTarget) &&
					   UnitGetTeam( pTarget ) != INVALID_TEAM && TestHostile( game, pUnit, pTarget ) ) ) )
				{
					UISetClickedTargetUnit( UnitGetId(pTarget), TRUE, FALSE, nSkillRight );
					if( PlayerIsInAttackRange( game, pTarget, nSkillRight, TRUE ) )
					{
						c_PlayerStopPath( game );
						c_PlayerMouseAimLocation( game, &pTarget->vPosition );
						sgbStartedRightSkill = TRUE;
						if( nSkillOverride != INVALID_ID )
						{
							sStartSkillRequestByID(game, nSkillOverride );
						}
						else
						{
							sStartSkillRequest(game, STATS_SKILL_RIGHT );
						}
						sgbInteracted = TRUE;
						sgbRightSkillActive = TRUE;
					}
					else
					{
						c_PlayerMoveToLocation( game, &pTarget->vPosition );
						if( nSkillOverride != INVALID_ID )
						{
							sEndSkillRequestByID(game, nSkillOverride );
						}
						else
						{
							sEndSkillRequest(game, STATS_SKILL_RIGHT);
						}
						
						sgbRightSkillActive = FALSE;
					}

				}
				else
				{
					UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );

					c_PlayerStopPath( game );
					c_PlayerMouseAim( game, TRUE );
					sgbStartedRightSkill = TRUE;
					if( nSkillOverride != INVALID_ID )
					{
						sStartSkillRequestByID(game, nSkillOverride );
					}
					else
					{
						sStartSkillRequest(game, STATS_SKILL_RIGHT );
					}
					sgbInteracted = TRUE;
					sgbRightSkillActive = TRUE;
				}
			}
			else
			{

				//end target
				UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, INVALID_ID );
				int nSkill = UnitGetStat(pUnit, STATS_SKILL_RIGHT);
				if( nSkillOverride != INVALID_ID )
				{
					nSkill = nSkillOverride;
				}
				const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );

				if ( pSkillData &&
					SkillDataTestFlag( pSkillData, SKILL_FLAG_PLAYER_STOP_MOVING ) )
				{
					c_PlayerStopPath( game );
					c_PlayerMouseAim( game, TRUE );
				}
				sgbStartedRightSkill = TRUE;
				if( nSkillOverride != INVALID_ID )
				{
					sStartSkillRequestByID(game, nSkillOverride );
				}
				else
				{
					sStartSkillRequest(game, STATS_SKILL_RIGHT );
				}
				sgbInteracted = TRUE;
				sgbRightSkillActive = TRUE;

			}

		}
	}
}

static void sHandleSendingHoverUnitToConsole( 
	GAME* game )
{
	if ( GetKeyState( VK_SHIFT ) & 0x8000 )
	{
		UNIT *pTarget = UnitGetById( game, UIGetHoverUnit() );
		if( pTarget )
		{
			WCHAR uszName[ 255 ];
			DWORD dwNameFlags( 0 );
			SETBIT( dwNameFlags, UNF_EMBED_COLOR_BIT );									
			UnitGetName( pTarget, uszName, 255, dwNameFlags );
			UIEditPasteString( uszName );		
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInputHandleRightButton(
	GAME* game,
	BOOL bButtonDown)
{
	if (AppCommonGetAbsTime() < gInput.m_tiBlockMouseBtnUntil)
	{
		return;
	}

	if( AppIsHellgate() )
	{
		if (UICursorGetActive())
		{
			if (ResultIsHandled(UISendMessage((bButtonDown ? WM_RBUTTONDOWN : WM_RBUTTONUP), 
				0, MAKE_PARAM((int)gInput.m_fMouseX, (int)gInput.m_fMouseY))))
			{
				return;
			}
			else
			{
				if (!sInRTS())
					return;
			}
		}

		if (AppGetState() == APP_STATE_IN_GAME && !AppCommonIsPaused() && game)	
		{
			if (bButtonDown)
			{
				UNIT *pPlayer = GameGetControlUnit( game );
				c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_LEFTBUTTON );
				UNIT *pTarget = UIGetTargetUnit();
				//sAbortVanity( game );
				if ( c_CameraGetViewType() >= VIEW_DIABLO )
				{
					sStartSkillRequest(game, STATS_SKILL_RIGHT);
				}
				else if ( pTarget && 
					(UnitCanInteractWith( pTarget, pPlayer ) || 
					(UnitIsPlayer(pTarget) && UnitIsInTown(pTarget))))
				{
					if (UnitIsNPC(pTarget) || UnitIsPlayer(pTarget))
					{
						c_InteractRequestChoices( pTarget );
					}
					else
					{
						c_PlayerInteract( game, pTarget );
					}
				}
				else
				{
					sStartSkillRequest(game, STATS_SKILL_RIGHT);
				}
			}
			else
			{
				sEndSkillRequest(game, STATS_SKILL_RIGHT);
			}
		}
		else if(AppGetState() == APP_STATE_PLAYMOVIELIST)
		{
			if(bButtonDown)
			{
				c_MoviePlayerSkipToEnd();
			}
		}
	}
	else   //tugboat
	{
		if (UICursorGetActive() || UIGetShowItemNamesState()) 
		{
	
			sHandleSendingHoverUnitToConsole(game);
			LPARAM lParam = MAKE_PARAM((int)gInput.m_fMouseX, (int)gInput.m_fMouseY);
			if( ResultIsHandled( UISendMessage((bButtonDown ? WM_RBUTTONDOWN : WM_RBUTTONUP), 0, lParam) ) )
				return;

		}
		
		sInputHandleRightButtonSkill( game, bButtonDown );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInputHandleMouseWheelDelta(
	int nDelta)
{
	if (!InputIsRemappingKey())
	{
		if (ResultStopsProcessing(UISendMessage(WM_MOUSEWHEEL, 0, (LPARAM)nDelta)))
		{
			return;
		}
	}

	InputHandleKey(AppGetCltGame(), WM_KEYDOWN, (nDelta > 0 ? HGVK_MOUSE_WHEEL_UP : HGVK_MOUSE_WHEEL_DOWN), 0);
	InputHandleKey(AppGetCltGame(), WM_KEYUP, (nDelta > 0 ? HGVK_MOUSE_WHEEL_UP : HGVK_MOUSE_WHEEL_DOWN), 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_InputTugboatMouseUpdate(
	GAME* game)
{
	sgbInteracted = FALSE;			
	sgbStartedLeftSkill = FALSE;	
	sgbStartedRightSkill = FALSE;	

	BOOL bInMainMenu = (AppGetState() != APP_STATE_IN_GAME);
	if( !InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) )
	{
		if( sgbPathingOnClick )
		{
			sgbPathingOnClick = FALSE;
		}
	}
	int nSkillLeft = INVALID_ID;		//Needed before for checks
	int nSkillRight = INVALID_ID;		//Needed before for checks
	if (!bInMainMenu &&  game && !AppCommonIsPaused() && 
		!IS_SERVER(game) &&
//			 CameraGetUpdated() &&
		 !sgbInteracted )
	{
		float fDeltaTime =  AppCommonGetElapsedTime();
		int nInput = c_PlayerGetMovement( game );
		UNIT* pUnit = GameGetControlUnit( game );
		BOOL bCanFollowCamera = TRUE;
		if( ( ( nInput & PLAYER_STRAFE_LEFT ) || ( nInput & PLAYER_STRAFE_RIGHT ) ) &&
			!( (nInput & PLAYER_MOVE_FORWARD ) || (nInput & PLAYER_MOVE_BACK ) ) )
		{
			bCanFollowCamera = FALSE;
			sgfFollowCameraTimer = 0;
		}
		if( ( nInput & PLAYER_ROTATE_LEFT ) || ( nInput & PLAYER_ROTATE_RIGHT ) )
		{
			bCanFollowCamera = FALSE;
		}
		if( ( nInput & PLAYER_MOVE_BACK ) )
		{
			bCanFollowCamera = FALSE;
			sgfFollowCameraTimer = 0;
		}
		if( InputIsCommandKeyDown( CMD_CAMERAORBIT ) )
		{
			sgfFollowCameraTimer = 0;
		}

		float fFollowAngle = VectorDirectionToAngle( UnitGetFaceDirection( pUnit, FALSE ) );
		

		const CAMERA_INFO * camera_info = c_CameraGetInfo(FALSE);
		if (camera_info && UnitPathIsActive(pUnit) )
		{
			VECTOR vDirection = CameraInfoGetDirection(camera_info);
			if( VectorDot( UnitGetFaceDirection( pUnit, FALSE ), vDirection ) <= 0 )
			{
				sgfFollowCameraTimer = 0;
			}
		}
		if( sgfFollowCameraTimer != 0 && UnitIsInAttack( pUnit ) )
		{
			sgfFollowCameraTimer = 0;
		}
	

		if ( InputIsCommandKeyDown( CMD_RESETCAMERAORBIT ) )
		{
			c_CameraResetFollow();
			sgfFollowCameraTimer = 0;
		}
		else if( (InputGetFollowCamera() && bCanFollowCamera ) && pUnit )
		{
			if( sgfFollowCameraTimer > 1.2f )
			{

				while ( fFollowAngle < 0.0f )
				{
					fFollowAngle += MAX_RADIAN;
				}
				while ( fFollowAngle >= MAX_RADIAN )
				{
					fFollowAngle -= MAX_RADIAN;				
				}
				c_CameraFollowLerp( fFollowAngle, fDeltaTime * .75f );
			}
		}

		sgfFollowCameraTimer += fDeltaTime;
		if( sgfFollowCameraTimer > 10 )
		{
			sgfFollowCameraTimer = 10;
		}

		BOOL OverPanel = UI_TB_MouseOverPanel();
		if( !OverPanel )
		{
			UIClearHoverText();
		}
		
		if( pUnit &&  !IsUnitDeadOrDying( pUnit ) &&
			!SkillIsOnWithFlag( pUnit, SKILL_FLAG_NO_PLAYER_MOVEMENT_INPUT ) )
		{
			LEVEL* pLevel = UnitGetLevel( pUnit );
			if( pUnit )
			{
				nSkillLeft = UnitGetStat( pUnit, STATS_SKILL_LEFT );
				nSkillRight = sgnLastRightSkill != INVALID_ID ? sgnLastRightSkill : UnitGetStat( pUnit, STATS_SKILL_RIGHT );
			}
			int nSkill = UIGetClickedTargetSkill();
			BOOL bLeft = UIGetClickedTargetLeft();

			
			UNIT *pTarget = UIGetClickedTargetUnit();

			// if we just switched levels, blank the target.
			if( pUnit && pTarget && UnitGetLevel( pUnit ) != UnitGetLevel( pTarget ) )
			{
				pTarget = NULL;
			}
			if ( InputIsCommandKeyDown( CMD_SKILL_SHIFT ) )
			{
				pTarget = NULL;
				UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, nSkillLeft );
				sgbPathingOnClick = TRUE;
			}

			// update to flaming sword icon if we're allowing
			// PVP aggression right now
			if ( InputIsCommandKeyDown( CMD_ENGAGE_PVP ) &&
				 !LevelIsSafe( pLevel ) && 
				 PlayerIsInPVPWorld( pUnit ) )
			{
				if( UIGetCursorState() == UICURSOR_STATE_POINTER ||
					UIGetCursorState() == UICURSOR_STATE_INITIATE_ATTACK )
				{
					UISetCursorState( UICURSOR_STATE_INITIATE_ATTACK );
				}
			}
			else
			{
				if( UIGetCursorState() == UICURSOR_STATE_INITIATE_ATTACK )
				{
					UISetCursorState( UICURSOR_STATE_POINTER );
				}
			}

			if ( c_CameraGetViewType() >= VIEW_3RDPERSON &&
				 c_CameraGetViewType() != VIEW_VANITY )
			{
				if ( pTarget )
				{
					if ( pTarget && UnitCanInteractWith( pTarget, pUnit )  )
					{
							float Distance = UNIT_INTERACT_INITIATE_DISTANCE_SQUARED;
							if( ObjectIsDoor( pTarget ) )
							{
								Distance *= 4;
							}
						if ( UnitsGetDistanceSquaredMinusRadiusXY( pUnit, pTarget ) < Distance )
						{

								sEndSkillRequestByID(game, nSkillLeft);
								sgbLeftSkillActive = FALSE;
								c_PlayerMouseAimLocation( game, &pTarget->vPosition ); 
								c_PlayerInteract( game, pTarget, UNIT_INTERACT_DEFAULT, TRUE );
								c_PlayerStopPath( game );
								sgfFollowCameraTimer = 0;
								// done interacting
								UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );
			
						}
						else
						{

								sEndSkillRequestByID(game, nSkillLeft);
								sgbLeftSkillActive = FALSE;
								if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) &&
									c_PlayerCanPath() )
								{
									c_PlayerMoveToLocation( game, &pTarget->vPosition );
								}

						}
					}
					else if ( UnitTestFlag(pUnit, UNITFLAG_CANPICKUPSTUFF) && UnitTestFlag( pTarget, UNITFLAG_CANBEPICKEDUP ) )
					{
						if ( UnitsGetDistanceSquaredXY( pUnit, pTarget ) < UNIT_INTERACT_INITIATE_DISTANCE_SQUARED * 2 )
						{

								sEndSkillRequestByID(game, nSkillLeft);
								sgbLeftSkillActive = FALSE;
								c_PlayerMouseAimLocation( game, &pTarget->vPosition );
								c_ActionKey( game );
								c_PlayerStopPath( game );
								sgfFollowCameraTimer = 0;
								// done interacting
								UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );

						}
						else
						{

								sEndSkillRequestByID(game, nSkillLeft);
								sgbLeftSkillActive = FALSE;
								if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) &&
									c_PlayerCanPath() )
								{
									c_PlayerMoveToLocation( game, &pTarget->vPosition );
								}

							
						}
					}
					else if( UnitIsA( pTarget, UNITTYPE_DESTRUCTIBLE ) ||
							 (  UnitGetTeam( pTarget ) != INVALID_TEAM && TestHostile( game, pUnit, pTarget ) ) )
					{
						if( !InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) &&
							bLeft &&
							!UnitPathIsActive( pUnit ) /*&&
							!UnitIsInAttack( pUnit )*/ )
						{
							// done interacting
							UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );
						}
						else if( !InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT ) &&
							!bLeft &&
							!UnitPathIsActive( pUnit ) /*&&  
							!UnitIsInAttack( pUnit ) */)
						{
							// done interacting
							UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );
						}
						else
						{
							if( ( UnitIsA( pTarget, UNITTYPE_DESTRUCTIBLE ) &&
								!IsUnitDeadOrDying(pTarget) ) ||
								( !IsUnitDeadOrDying(pTarget) &&
								  (  UnitGetTeam( pTarget ) != INVALID_TEAM && TestHostile( game, pUnit, pTarget ) ) ) )
							{
								const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );

								BOOL bCanTarget = FALSE;
								if ( pSkillData &&
									 SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) )
								{
									bCanTarget = TRUE;
								}
								// try to kick it if it is destructible
								if( bLeft && UnitIsA( pTarget, UNITTYPE_DESTRUCTIBLE )  )
								{
									if( PlayerIsInAttackRange( game, pTarget, UnitGetData( pUnit )->nKickSkill, TRUE ) )
									{
										c_PlayerStopPath( game );
										c_PlayerMouseAimLocation( game, &pTarget->vPosition );

										sStartSkillRequestByID( game, UnitGetData( pUnit )->nKickSkill );
										sEndSkillRequestByID( game, UnitGetData( pUnit )->nKickSkill );
									}
									else
									{
										sEndSkillRequestByID(game, nSkill == nSkillLeft ? nSkillLeft : nSkillRight );
										if( nSkillLeft == nSkill )
										{
											sgbLeftSkillActive = FALSE;
										}
										else if( nSkillRight == nSkill )
										{
											sgbRightSkillActive = FALSE;
										}
										if( c_PlayerCanPath() && bLeft )
										{
											c_PlayerMoveToLocation( game, &pTarget->vPosition );
										}
									}
								}
								else if( PlayerIsInAttackRange( game, pTarget, nSkill, TRUE ) ||
									!bCanTarget )
								{
									c_PlayerStopPath( game );
									c_PlayerMouseAimLocation( game, &pTarget->vPosition );

									sStartSkillRequestByID( game, nSkill == nSkillLeft ? nSkillLeft : nSkillRight );
									if( nSkillLeft == nSkill )
									{
										sgbStartedLeftSkill = TRUE;
										sgbLeftSkillActive = TRUE;
									}
									else if( nSkillRight == nSkill )
									{
										sgbStartedRightSkill = TRUE;
										sgbRightSkillActive = TRUE;
									}
								}
								else
								{

										sEndSkillRequestByID(game, nSkill == nSkillLeft ? nSkillLeft : nSkillRight );
										if( nSkillLeft == nSkill )
										{
											sgbLeftSkillActive = FALSE;
										}
										else if( nSkillRight == nSkill )
										{
											sgbRightSkillActive = FALSE;
										}
										if( c_PlayerCanPath() )
										{
											c_PlayerMoveToLocation( game, &pTarget->vPosition );
										}
									
								}

							}
							else
							{
								// done interacting
								UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );
								if( !sgbStartedLeftSkill )
								{
									if ( sgbLeftSkillActive )
									{
										sEndSkillRequestByID(game, nSkillLeft);
										sgbLeftSkillActive = FALSE;
									}
								}
								if( !sgbStartedRightSkill )
								{
									if ( sgbRightSkillActive && !InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT ) )
									{
										sEndSkillRequestByID(game, nSkillRight);
										sgbRightSkillActive = FALSE;
									}
								}
							}
						}

						

					}
					else
					{

							sEndSkillRequestByID(game, nSkillLeft);
							sgbLeftSkillActive = FALSE;
							sEndSkillRequest(game, STATS_SKILL_RIGHT );
							sgbRightSkillActive = FALSE;
							if( c_PlayerCanPath() )
							{
								c_PlayerMoveToLocation( game, &pTarget->vPosition );
							}

							UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );

					}
				}
				else
				{

					UISetClickedTargetUnit( INVALID_ID, FALSE, TRUE, INVALID_ID );
					
					if ( !OverPanel && InputIsCommandKeyDown( CMD_SKILL_SHIFT ) )
					{
						UISetClickedTargetUnit( INVALID_ID, TRUE, TRUE, nSkillLeft );
						c_PlayerStopPath( game );
						sgbPathingOnClick = TRUE;
						if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) ||
							InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT ))
						{
							c_PlayerMouseAim( game, TRUE );
							sgbStartedLeftSkill = TRUE;
							sStartSkillRequestByID(game, nSkillLeft );
							sgbLeftSkillActive = TRUE;
						}
					}
					else if (  !OverPanel && ( InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT )  ||
											   sgbSpellHotkeyDown ) &&
											 sgbRightSkillActive)
					{

							const SKILL_DATA * pSkillData = SkillGetData( game, nSkillRight );

							if( !SkillDataTestFlag( pSkillData, SKILL_FLAG_START_ON_SELECT ) && 
								( SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_ALL ) ||
								 SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_FIRE ) ) )
							{
								c_PlayerMouseAim( game, TRUE );
								sgbStartedRightSkill = TRUE;
								sStartSkillRequestByID(game, nSkillRight );
								sgbRightSkillActive = TRUE;
							}
							else
							{
								if( sgbRightSkillActive )
								{
									sEndSkillRequestByID(game, nSkillRight );
									sgbRightSkillActive = FALSE;
								}
								if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) &&
									sgbPathingOnClick )
								{
									if( c_PlayerCanPath() )
									{									
										c_PlayerMoveToMouse( game, FALSE );
									}
								}
							}
					}
					else if( !OverPanel )
					{
						if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) &&
							sgbPathingOnClick )
						{

								sEndSkillRequestByID(game, nSkillLeft );
								sgbLeftSkillActive = FALSE;
								if( c_PlayerCanPath() )
								{									
									c_PlayerMoveToMouse( game, FALSE );
								}


						}
					}
				}
			}
		}
	}
	
	if( !bInMainMenu && !AppCommonIsPaused() && !InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) )
	{

		if ( sgbLeftSkillActive && !sgbStartedLeftSkill )
		{
			sEndSkillRequestByID(game, nSkillLeft );
			sgbLeftSkillActive = FALSE;
		}

	}
	if( !bInMainMenu && !AppCommonIsPaused() && !InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT ))
	{
		if ( sgbRightSkillActive && !sgbStartedRightSkill && !sgbSpellHotkeyDown )
		{
			sEndSkillRequestByID(game, nSkillRight );
			sgbRightSkillActive = FALSE;
		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DirectInputUpdate(
	GAME* game)
{
	if (c_UsingDirectInput() && InputGetMouseAcquired())
	{

		// am I in cinematic/story mode?
		if (game)
		{
			UNIT * player = GameGetControlUnit(game);
			if (player && UnitTestFlag(player, UNITFLAG_QUESTSTORY))
			{
				return;
			}
		}

		static BOOL bBufferedMouseButtons = FALSE;

		// Get the buffered mouse events (we will use this for mouse positioning)
		DIDEVICEOBJECTDATA DeviceObjectData[MOUSE_BUFFER_SIZE];
		DWORD dwElements = MOUSE_BUFFER_SIZE;   
		PRESULT pr = S_OK;
		V_SAVE( pr, gInput.m_pMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), DeviceObjectData, &dwElements, 0) );
		if (pr == DIERR_INPUTLOST || pr == DIERR_NOTACQUIRED)
		{
			c_InputMouseAcquire(game, TRUE);
			return;
		}
		ASSERT(pr == DI_OK || pr == DI_BUFFEROVERFLOW);

		BOOL bInMainMenu = (AppGetState() == APP_STATE_MAINMENU || AppGetState() == APP_STATE_CHARACTER_CREATE);
		REF( bInMainMenu );
		BOOL bMouseMove = FALSE;
		float fMouseX, fMouseY;
		InputGetMousePosition(&fMouseX, &fMouseY);

		for (DWORD ii = 0; ii < dwElements; ++ii)
		{
			switch (DeviceObjectData[ii].dwOfs)
			{
			case DIMOFS_X:
				{
					float delta = (float)DeviceObjectData[ii].dwData;
					if (UICursorGetActive())
					{
						bMouseMove = TRUE;
						fMouseX += (float)delta * gInput.m_fMouseSensitivity;
					}
					else if (!AppCommonIsPaused())
					{
						delta = ((float)delta * gInput.m_fMouseLookSensitivity);
						PlayerMouseRotate(game, delta);
					}
				}
				break;

			case DIMOFS_Y:
				{
					float delta = (float)DeviceObjectData[ii].dwData;
					if (UICursorGetActive())
					{
						bMouseMove = TRUE;
						fMouseY += (float)delta * gInput.m_fMouseSensitivity;
					}
					else if (!AppCommonIsPaused())
					{
						delta = ((float)delta * gInput.m_fMouseLookSensitivity);
						if (!InputGetMouseInverted())
						{
							PlayerMousePitch(game, -delta);
						}
						else
						{
							PlayerMousePitch(game, delta);
						}
					}
				}
				break; 

			case DIMOFS_Z:			// mouse wheel
				sInputHandleMouseWheelDelta((int)DeviceObjectData[ii].dwData);
				break;

			case DIMOFS_BUTTON0:	// left mouse button  (mouse down: (data != 0), mouse up: (data == 0)
				if (bBufferedMouseButtons)
				{
					sInputHandleLeftButton(game, DeviceObjectData[ii].dwData != 0);
				}
				break;

			case DIMOFS_BUTTON1:	// right mouse button
				if (bBufferedMouseButtons)
				{
					sInputHandleRightButton(game, DeviceObjectData[ii].dwData != 0);
				}
				break;

			case DIMOFS_BUTTON3:	// thumb mouse button
				if ( AppIsHellgate() && ! bInMainMenu )
				{
					SETBIT(&gInput.m_dwMouseButtonState, MOUSE_BUTTON_X1, DeviceObjectData[ii].dwData != 0);
					if ( TESTBIT( gInput.m_dwMouseButtonState, MOUSE_BUTTON_X1 ) )
						c_PlayerToggleMovement( game, PLAYER_AUTORUN );
				}
				break;

			}
 		}

		if (bMouseMove)
		{
			InputSetMousePosition(fMouseX, fMouseY);
		}


		if (!bBufferedMouseButtons)
		{
			// get the immediate state of the mouse buttons
			DIMOUSESTATE diMouseState;
			PRESULT pr = S_OK;
			V_SAVE( pr, gInput.m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &diMouseState) );
			if (pr == DIERR_INPUTLOST || pr == DIERR_NOTACQUIRED)
			{
				c_InputMouseAcquire(game, TRUE);
				return;
			}
			ASSERT(pr == DI_OK || pr == DI_BUFFEROVERFLOW);

			if( gInput.m_bJustActivated )
			{
				if(  (diMouseState.rgbButtons[MOUSE_BUTTON_LEFT] & 0x80) == 0  )
				{
					gInput.m_bJustActivated = FALSE;
				}
				else
				{
					return;
				}
			}
			// if the mouse button states have changed from our last recorded states, handle the event
			if (((diMouseState.rgbButtons[MOUSE_BUTTON_LEFT] & 0x80) != 0) != TESTBIT(gInput.m_dwMouseButtonState, MOUSE_BUTTON_LEFT))
			{
				sInputHandleLeftButton(game, (diMouseState.rgbButtons[MOUSE_BUTTON_LEFT] & 0x80) != 0);
			}
			if (((diMouseState.rgbButtons[MOUSE_BUTTON_RIGHT] & 0x80) != 0) != TESTBIT(gInput.m_dwMouseButtonState, MOUSE_BUTTON_RIGHT))
			{
				sInputHandleRightButton(game, (diMouseState.rgbButtons[MOUSE_BUTTON_RIGHT] & 0x80) != 0);
			}
		}
	}

	if( !AppIsHellgate() )
	{
		c_InputTugboatMouseUpdate(game);
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GetMouseButtonStateImmediate(
	BYTE *pByte)
{
	if (!c_UsingDirectInput())
	{
		memcpy(pByte, (void *)(&gInput.m_ulRawButtons), sizeof(BYTE) * 4);
	}
	else
	{
		// get the immediate state of the mouse buttons
		DIMOUSESTATE diMouseState;
		PRESULT pr = S_OK;
		V_SAVE( pr, gInput.m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &diMouseState) );
		if (pr == DIERR_INPUTLOST || pr == DIERR_NOTACQUIRED)
		{
			c_InputMouseAcquire(AppGetCltGame(), TRUE);
			return FALSE;
		}
		ASSERT(pr == DI_OK || pr == DI_BUFFEROVERFLOW);

		memcpy(pByte, diMouseState.rgbButtons, sizeof(BYTE) * 4);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrRotLeftDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, PLAYER_ROTATE_LEFT);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrRotLeftUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_ROTATE_LEFT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrRotRightDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	c_PlayerSetMovement(game, PLAYER_ROTATE_RIGHT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrRotRightUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_ROTATE_RIGHT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyForwardDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_TutorialInputOk( unit, TUTORIAL_INPUT_MOVE_FORWARD );
	c_PlayerClearMovement(game, PLAYER_AUTORUN);
	c_PlayerSetMovement(game, PLAYER_MOVE_FORWARD);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyForwardUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_MOVE_FORWARD);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyBackDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_TutorialInputOk( unit, TUTORIAL_INPUT_MOVE_BACK );
	if( AppIsHellgate() )
	{
		c_PlayerClearMovement(game, PLAYER_AUTORUN);
		c_PlayerSetMovement(game, PLAYER_MOVE_BACK);
	}
	else
	{
		if( c_PlayerTestMovement( PLAYER_AUTORUN ) )
		{
			c_PlayerClearMovement(game, PLAYER_AUTORUN);
		}
		else
		{
			c_PlayerSetMovement(game, PLAYER_MOVE_BACK);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyBackUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_MOVE_BACK);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrafeLeftDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_TutorialInputOk( unit, TUTORIAL_INPUT_MOVE_SIDES );
	c_PlayerSetMovement(game, PLAYER_STRAFE_LEFT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrafeLeftUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_STRAFE_LEFT);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrafeRightDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_TutorialInputOk( unit, TUTORIAL_INPUT_MOVE_SIDES );
	c_PlayerSetMovement(game, PLAYER_STRAFE_RIGHT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyStrafeRightUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_STRAFE_RIGHT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCameraZoom(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	BOOL bInMainMenu = (AppGetState() == APP_STATE_MAINMENU || AppGetState() == APP_STATE_CHARACTER_CREATE);
	if (!bInMainMenu && !AppCommonIsPaused())
	{
		int nDelta = (int)data;
		if( AppIsHellgate() )
		{
			if (c_CameraGetViewType() == VIEW_VANITY)
			{
				if (nDelta > 0)
				{
					c_CameraVanityDistance(TRUE);
				}
				else
				{
					c_CameraVanityDistance(FALSE);
				}
			}
			else
			{
				UNIT * player = GameGetControlUnit( game );
				if ( player )
				{
					c_CameraZoom( nDelta < 0 );
				}
			}
		}
		else //tugboat
		{
			if (c_CameraGetViewType() == VIEW_VANITY)
			{
				if (nDelta > 0)
				{
					c_CameraVanityDistance(TRUE);
				}
				else
				{
					c_CameraVanityDistance(FALSE);
				}
			}
			else
			{
				 if (nDelta > 0)
				 {
					 GAME* game = AppGetCltGame();
					 if (game)
					 {
						 UNIT* unit = GameGetControlUnit(game);
						 if (unit)
						 {
							 c_CameraDollyIn();
						 }
					 }
				 }
				 else
				 {
					 c_CameraDollyOut(); 
				 }
			}
		}	
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCameraZoomCharacterCreate(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE( AppGetState() == APP_STATE_CHARACTER_CREATE );
	ASSERT_RETFALSE( AppIsHellgate() );
	ASSERT_RETFALSE( c_CameraGetViewType() == VIEW_CHARACTER_SCREEN );

	if ( AppCommonIsPaused() )
		return FALSE;

	int nDelta = (int)data;
	c_CameraCharacterScreenDistance( nDelta > 0 );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyJumpDown(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	ASSERT_RETFALSE(game && unit);
	c_TutorialInputOk( unit, TUTORIAL_INPUT_SPACEBAR );
	c_PlayerSetMovement(game, PLAYER_JUMP);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyJumpUp(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_JUMP);
	return TRUE;
}





//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyInventoryScreen(
	KEY_HANDLER_PARAMS)
{
	
	if( AppIsHellgate() )
	{
		ASSERT_RETFALSE(game && unit);
		// moved to InventoryOnPostActivate [mk]
//		UIStopAllSkillRequests();
//		c_PlayerClearAllMovement(AppGetCltGame());
		if (!UIComponentGetActive(UICOMP_INVENTORY))
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_INVENTORY );
			UNIT * pPlayer = GameGetControlUnit( game );
			UNITID idPlayer = UnitGetId(pPlayer);
			UIComponentActivate(UICOMP_INVENTORY, TRUE);	
			UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
			UIComponentSetFocusUnitByEnum(UICOMP_CHARSHEET, idPlayer, TRUE);
		}
		else
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_INVENTORY_CLOSE );
			UIComponentActivate(UICOMP_INVENTORY, FALSE);	
		}
	}
	else
	{
		ASSERT_RETFALSE(game && unit);
		
		UITogglePaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );				
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCharacterScreen(
	KEY_HANDLER_PARAMS )
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}
	ASSERT_RETFALSE(game && unit);
	
	UITogglePaneMenu( KPaneMenuCharacterSheet );				
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAutoParty(
	KEY_HANDLER_PARAMS )
{
	if( AppIsHellgate() )
	{
		ASSERT_RETFALSE(game && unit);
		c_PlayerToggleAutoParty( unit );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySkillMapScreen(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		ASSERT_RETFALSE(game && unit);
		// moved to UIOnPostActivateSkillMap [mk]
//		UIStopAllSkillRequests();
//		c_PlayerClearAllMovement(AppGetCltGame());
		BOOL bIsActive = UIComponentGetActive(UICOMP_SKILLMAP);
		if ( !bIsActive )
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_SKILLS );
		}
		else
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_SKILLS_CLOSE );
		}
		UIComponentActivate(UICOMP_SKILLMAP, !bIsActive);	
		UNIT * pPlayer = GameGetControlUnit( game );
		UNITID idPlayer = UnitGetId(pPlayer);
		UIComponentSetFocusUnitByEnum(UICOMP_CHARSHEET, idPlayer, TRUE);
	}
	else
	{
		
		ASSERT_RETFALSE(game && unit);
		UITogglePaneMenu( KPaneMenuSkills );		

	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyPerkMapScreen(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	BOOL bIsActive = UIComponentGetActive(UICOMP_PERKMAP);
	UIComponentActivate(UICOMP_PERKMAP, !bIsActive);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyBuddyList(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UIToggleBuddylist();		

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyGuildPanel(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UIToggleGuildPanel();		

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyPartyPanel(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UITogglePartyPanel();		

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyEmail(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UIToggleEmail();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAuctionHouse(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

#if ISVERSION(DEVELOPMENT)
	UIToggleAuctionHouse();
#endif

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyEmailScrollUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UIEmailScrollUp();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyEmailScrollDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UIEmailScrollDown();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyChatToggle(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
//		if (!AppGetType() != APP_TYPE_SINGLE_PLAYER)
		{
			//if (UICSRPanelIsOpen())
			//{
			//	return FALSE;
			//}

			extern UI_MSG_RETVAL UIChatMinimizeOnClick( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );

			UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_CHAT_MINIMIZE_BUTTON);
			ASSERT_RETFALSE(pComponent);
			UI_BUTTONEX *pButton = UICastToButton(pComponent); 

			UIButtonSetDown(pButton, !UIButtonGetDown(pButton));
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);
			UIChatMinimizeOnClick(pButton, 0, 0, 0);

			//ASSERT_RETFALSE(game && unit);
			////UIShowSocialScreen(UI_SOCIALPANEL_CHAT);		
			//UIComponentActivate(UICOMP_SOCIAL_SCREEN, TRUE);
		}
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyHideSocialScreen(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		ASSERT_RETFALSE(game && unit);
//		UIHideSocialScreen(command == CMD_UI_QUICK_EXIT);		
		UIComponentActivate(UICOMP_SOCIAL_SCREEN, FALSE);
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDemoModeInterrupt(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE( NULL != DemoLevelGetDefinition() );

	DemoLevelInterrupt();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sTakeScreenshot(
	KEY_HANDLER_PARAMS)
{
#if ISVERSION(DEVELOPMENT)
	if (ConsoleIsEditActive())
	{
		return FALSE;
	}
#endif
	// if control is down, take screenshot without UI
	SHORT nKeyState = GetKeyState(VK_CONTROL);

	SCREENSHOT_PARAMS tParams;
	tParams.bWithUI = ! (nKeyState & 0x8000);
	tParams.bSaveCompressed = e_GetScreenshotCompress();
	e_CaptureScreenshot( tParams );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCrouchDown(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, PLAYER_CROUCH);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCrouchUp(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_CROUCH);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sTakeScreenshotBurst(
	KEY_HANDLER_PARAMS)
{
	if (ConsoleIsEditActive())
	{
		return FALSE;
	}

	// if control is down, take screenshot without UI
	SHORT nKeyState = GetKeyState(VK_CONTROL);

	SCREENSHOT_PARAMS tParams;
	tParams.bWithUI = ! (nKeyState & 0x8000);
	tParams.bBurst = TRUE;
	tParams.bSaveCompressed = e_GetScreenshotCompress();
	e_CaptureScreenshot( tParams );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDebugUp(
	KEY_HANDLER_PARAMS)
{
	DebugKeyUp();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDebugDown(
	KEY_HANDLER_PARAMS)
{
	DebugKeyDown();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sScreenshotNOP(
	KEY_HANDLER_PARAMS)
{
	// this is a NOP stub to override any other registered actions for this key event
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAutomap(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if ( AppIsHellgate() )
	{
		c_TutorialInputOk( unit, TUTORIAL_INPUT_AUTOMAP );
	}
	UICycleAutomap();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyWeaponSetSwap(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	MSG_CCMD_REQ_WEAPONSET_SWAP msg;
	msg.wSet = (short)!UnitGetStat(unit, STATS_ACTIVE_WEAPON_SET);
	c_SendMessage(CCMD_REQ_WEAPONSET_SWAP, &msg);

	return TRUE;
}

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyMainControlsDown(
	KEY_HANDLER_PARAMS)
{
	UISetCinematicMode(FALSE);
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAutoMapZoomIn(
	KEY_HANDLER_PARAMS)
{
	UIAutoMapZoom(-1);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAutoMapZoomOut(
	KEY_HANDLER_PARAMS)
{
	UIAutoMapZoom(1);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyResetCameraOrbit(
	 KEY_HANDLER_PARAMS)
{

	c_CameraResetFollow();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCameraOrbitDown(
	KEY_HANDLER_PARAMS)
{
	if( gPreOrbitCursorPosX == -1 &&
		gPreOrbitCursorPosY == -1 )
	{
		POINT pt;
		::GetCursorPos(&pt);
		gPreOrbitCursorPosX = pt.x;
		gPreOrbitCursorPosY = pt.y;
	}
	::SetCursorPos(gInput.m_nScreenCenterX, gInput.m_nScreenCenterY);
	if (c_CameraGetViewType() == VIEW_VANITY)
	{

		UIUpdateHardwareCursor();
	}


	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCameraOrbitUp(
	KEY_HANDLER_PARAMS)
{
	::SetCursorPos(gPreOrbitCursorPosX, gPreOrbitCursorPosY);
	UIUpdateHardwareCursor();
	gPreOrbitCursorPosX = -1;
	gPreOrbitCursorPosY = -1;
	if (c_CameraGetViewType() == VIEW_VANITY)
	{
		UISetCursorState( UICURSOR_STATE_POINTER, 0, TRUE );

	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyShowItemsDown(
	KEY_HANDLER_PARAMS)
{
	//trace( "ALT - DOWN\n" );
	if( AppIsHellgate() )
	{
		UIStopAllSkillRequests();
		UIToggleShowItemNames(TRUE);

		if (command == CMD_SHOW_ITEMS)
		{
			UIShowAltMenu(TRUE);
		}
	}
	else
	{
		UIToggleShowItemNames( TRUE );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyShowItemsUp(
	KEY_HANDLER_PARAMS)
{
	//trace( "ALT - UP\n" );
	if( AppIsHellgate() )
	{
		UIToggleShowItemNames(FALSE);
		UIShowAltMenu(FALSE);
		UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
		if (pChat)
		{
			extern UI_MSG_RETVAL UIChatFadeOut( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
			UIChatFadeOut(pChat, NULL, 0, 0);
		}
	}
	else
	{
		UIToggleShowItemNames( FALSE );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyItemPickup(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		BOOL bInRange = FALSE;
		UNIT *pClosest = UIGetClosestLabeledItem(&bInRange);
		if (pClosest && bInRange)
		{
			if ( c_ItemTryPickup( game, pClosest->unitid ) )
			{
				UIPlayItemPickupSound(pClosest->unitid);
			}
		}
	}
	else
	{
		ALTITEMLIST * pClosest = c_GetCurrentClosestItem();
		if (pClosest->idItem != INVALID_ID && pClosest->bInPickupRange)
		{
			if ( c_ItemTryPickup( game, pClosest->idItem ) )
				UIPlayItemPickupSound(pClosest->idItem);
		}

	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySkillShiftDown(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		c_SkillsActivatorKeyDown( SKILL_ACTIVATOR_KEY_SHIFT );
	}
	else
	{
		if (!AppGetCltGame() )
		{
			return FALSE;
		}

		c_PlayerSetMovement(AppGetCltGame(), PLAYER_HOLD);

	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySkillShiftUp(
	KEY_HANDLER_PARAMS)
{

	if( AppIsTugboat() && AppGetCltGame() )
	{
		c_PlayerClearMovement(AppGetCltGame(), PLAYER_HOLD);

		// if we let go of shift, quickly see if we need to reaquire a target,
		// so we don't immediately go pathing off.
		CameraSetUpdated( TRUE );
		c_UIUpdateSelection( AppGetCltGame() );
		// we sort of treat this as a click event
		if( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) )
		{
			sInputHandleLeftButton( AppGetCltGame(), TRUE );
		}
		else if( InputGetMouseButtonDown( MOUSE_BUTTON_RIGHT ) )
		{
			sInputHandleRightButton( AppGetCltGame(), TRUE );
		}
	}

	c_SkillsActivatorKeyUp( SKILL_ACTIVATOR_KEY_SHIFT );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySkillPotionDown(
	KEY_HANDLER_PARAMS)
{
	c_SkillsActivatorKeyDown( SKILL_ACTIVATOR_KEY_POTION );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySkillPotionUp(
	KEY_HANDLER_PARAMS)
{
	c_SkillsActivatorKeyUp( SKILL_ACTIVATOR_KEY_POTION );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAutorun(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	c_PlayerToggleMovement(game, PLAYER_AUTORUN);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyForceMove(
	KEY_HANDLER_PARAMS)
{

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyQuickClose(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		ASSERT_RETFALSE(game && unit);

		if (UIComponentGetActive(UICOMP_INVENTORY))
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_INVENTORY_CLOSE );
		}
		if (UIComponentGetActive(UICOMP_QUEST_PANEL))
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_QUESTLOG_CLOSE );
		}
		if (UIComponentGetActive(UICOMP_SKILLMAP))
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_SKILLS_CLOSE );
		}
		if (UIComponentGetActive(UICOMP_WORLD_MAP_HOLDER))
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_WORLDMAP_CLOSE );
		}
	}

	return UIQuickClose();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyQuestLog(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if( AppIsHellgate() )
	{
		// moved to UIQuestPanelOnPostActivate [mk]
//		UIStopAllSkillRequests();
//		c_PlayerClearAllMovement(AppGetCltGame());
		BOOL bIsActive = UIComponentGetActive(UICOMP_QUEST_PANEL);
		if ( !bIsActive )
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_QUESTLOG );
		}
		else
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_QUESTLOG_CLOSE );
		}
		UIComponentActivate(UICOMP_QUEST_PANEL, !bIsActive);	
	}
	else
	{
		
		UITogglePaneMenu( KPaneMenuQuests );
	}
	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCrafting(
	KEY_HANDLER_PARAMS)
{

	ASSERT_RETFALSE(game && unit);
	UITogglePaneMenu( KPaneMenuCrafting, KPaneMenuCraftingRecipes, KPaneMenuBackpack );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyAchievements(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if( AppIsHellgate() )
	{
		// These are in UIAchievementsOnPostActivate
//		UIStopAllSkillRequests();
//		c_PlayerClearAllMovement(AppGetCltGame());
		BOOL bIsActive = UIComponentGetActive(UICOMP_ACHIEVEMENTS);
		UIComponentActivate(UICOMP_ACHIEVEMENTS, !bIsActive);	
	}
	else
	{
		UITogglePaneMenu( KPaneMenuAchievements );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyOpenFullLog(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	gbFullQuestLog = TRUE;
	UIUpdateQuestLog( QUEST_LOG_UI_UPDATE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCloseFullLog(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	gbFullQuestLog = FALSE;
	UIUpdateQuestLog( QUEST_LOG_UI_UPDATE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyWorldMap(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if ( AppIsHellgate() )
	{
		BOOL bIsActive = UIComponentGetActive(UICOMP_WORLD_MAP_HOLDER);
		if ( !bIsActive )
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_WORLDMAP );
		}
		else
		{
			c_TutorialInputOk( unit, TUTORIAL_INPUT_WORLDMAP_CLOSE );
		}
	}
	UIToggleWorldMap();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
//void drbSpline();
BOOL sKeyDRBDebug( KEY_HANDLER_PARAMS )
{
	if( AppIsTugboat() )
		return FALSE;

	//drbSpline();
	return TRUE;
}
//----------------------------------------------------------------------------

//void drbNewCollision();
static BOOL sgbMovieMode = FALSE;

BOOL sKeyDRBDebug2( KEY_HANDLER_PARAMS )
{
	if( AppIsTugboat() )
		return FALSE;

	//drbNewCollision();

/*	sgbMovieMode = !sgbMovieMode;
	if ( sgbMovieMode )
	{
		GAME * pGame = AppGetCltGame();
		UNIT * pPlayer = GameGetControlUnit( pGame );
		LEVEL * pLevel = UnitGetLevel( pPlayer );
		TARGET_TYPE eTT[] = { TARGET_GOOD, TARGET_INVALID };
		UNIT * pMurmur = LevelFindFirstUnitOf( pLevel, eTT, GENUS_MONSTER, GlobalIndexGet( GI_MONSTER_MURMUR ) );
		if ( !pMurmur )
			return TRUE;

		NPC_DIALOG_SPEC spec;
		spec.idTalkingTo = UnitGetId( pMurmur );
		spec.nDialog = DIALOG_TUTORIAL_MURMUR_TALK;
		spec.eType = NPC_DIALOG_OK_CANCEL;
		UIDisplayNPCStory( AppGetCltGame(), &spec );
	}
	else
	{
		UISetCinematicMode( FALSE );
	}*/
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyVanityCamDown(
	KEY_HANDLER_PARAMS)
{
	if (gInput.m_bVanityDown)
	{
		return TRUE;
	}

	ASSERT_RETFALSE(game && unit);
	if (c_CameraGetViewType() != VIEW_VANITY)
	{
		c_CameraSetViewType(VIEW_VANITY, TRUE);
	}
	else
	{
		c_CameraRestoreViewType();
	}
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);
	c_PlayerClearAllMovement( game );

	gInput.m_bVanityDown = TRUE;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyVanityCamUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	gInput.m_bVanityDown = FALSE;

	return TRUE;
}


#if (ISVERSION(DEVELOPMENT))
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySingleStepDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) || AppIsMultiplayer())
	{
		return FALSE;
	}
	AppSingleStep();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyResumeSpeedDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) || AppIsMultiplayer())
	{
		return FALSE;
	}
	AppUserPause(FALSE);
	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Resume normal speed");
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySlowerSpeedDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) || AppIsMultiplayer())
	{
		return FALSE;
	}
	
	int curspeed = AppCommonGetStepSpeed();
	int newspeed = AppCommonSetStepSpeed(curspeed + 1);
	if (curspeed != newspeed)
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"Simulation rate set to %s", AppCommonGetStepSpeedString());
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyFasterSpeedDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) || AppIsMultiplayer())
	{
		return FALSE;
	}
	int curspeed = AppCommonGetStepSpeed();
	int newspeed = AppCommonSetStepSpeed(curspeed - 1);
	if (curspeed != newspeed)
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"Simulation rate set to %s", AppCommonGetStepSpeedString());
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySimulateBadFPS(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) || AppIsMultiplayer())
	{
		return FALSE;
	}

	GameToggleDebugFlag(game, DEBUGFLAG_SIMULATE_BAD_FRAMERATE);
	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Simulate Bad Framerate %s", GameGetDebugFlag(game, DEBUGFLAG_SIMULATE_BAD_FRAMERATE) ? L"ON" : L"OFF");
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDetachCamera(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}
	AppSetDetachedCamera(!AppGetDetachedCamera());
	ConsoleString(CONSOLE_SYSTEM_COLOR, L"Detached camera set to: %s", AppGetDetachedCamera() ? L"ON" : L"OFF");
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySelectDebugUnit(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	const CAMERA_INFO * camera_info = c_CameraGetInfo(FALSE);
	if (!camera_info)
	{
		return FALSE;
	}
	VECTOR vPosition = CameraInfoGetPosition(camera_info);
	VECTOR vLookAt = CameraInfoGetLookAt(camera_info);
	VECTOR vDirection = CameraInfoGetDirection(camera_info);

	UNIT * selection = NULL;
	if (UICursorGetActive())
	{
		UNITID idUnit = UIGetHoverUnit();
		if (idUnit != INVALID_ID)
		{
			selection = UnitGetById(game, idUnit);
		}
		else
		{
			selection = c_UIGetMouseUnit( game );
		}
	}
	else
	{
		selection = SelectDebugTarget(game, unit, vPosition, vDirection, 500.0f);
	}
	GameSetDebugUnit(game, selection);
	c_SendDebugUnitId(UnitGetId(selection));
	if (selection)
	{
#if ISVERSION(DEBUG_ID)
		ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: %d [%s]", UnitGetId(selection).m_dwId, UnitGetId(selection).m_szName);
#else
		ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: %d [%s]", selection->unitid, UnitGetDevName(selection));
#endif
	}
	else
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: NO UNIT");
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeySelectDebugSelf(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	GameSetDebugUnit(game, unit);
	c_SendDebugUnitId(UnitGetId(unit));
	if (unit)
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: %d [%s]", (DWORD)UnitGetId(unit), UnitGetDevName(unit));
	}
	else
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, "Select debug unit: NO UNIT");
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT))
BOOL sKeyDebugTextLength(
	KEY_HANDLER_PARAMS)
{
	AppToggleDebugFlag(ADF_TEXT_LENGTH);
	UIRefreshText();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDebugTextLabels(
	KEY_HANDLER_PARAMS)
{
	AppToggleDebugFlag(ADF_TEXT_LABELS);
	UIRefreshText();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDebugUIClipping(
	KEY_HANDLER_PARAMS)
{
	AppToggleDebugFlag(ADF_UI_NO_CLIPPING);
	UIRefreshText();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDebugTextPointSize(
	KEY_HANDLER_PARAMS)
{
	AppToggleDebugFlag(ADF_TEXT_POINT_SIZE);
	UIRefreshText();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDebugUIEdit(
	KEY_HANDLER_PARAMS)
{
	GameToggleDebugFlag(game, DEBUGFLAG_UI_EDIT_MODE);
	BOOL bEnabled = GameGetDebugFlag(game, DEBUGFLAG_UI_EDIT_MODE);
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_UIEDIT, (LPARAM)bEnabled);

	ConsoleString(CONSOLE_SYSTEM_COLOR, L"UI Debug Edit: %s", (bEnabled ? L"ON" : L"OFF"));
		
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDebugUIReload(
	KEY_HANDLER_PARAMS)
{
	c_sUIReload(TRUE);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDebugTextDeveloper(
	KEY_HANDLER_PARAMS)
{
	AppToggleDebugFlag(ADF_TEXT_DEVELOPER);
	UIRefreshText();
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT))
BOOL sKeyDCForwardDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, PLAYER_MOVE_FORWARD);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCForwardUp(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_MOVE_FORWARD);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCBackDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, PLAYER_MOVE_BACK);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCBackUp(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_MOVE_BACK);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCStrafeLeftDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, PLAYER_STRAFE_LEFT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCStrafeLeftUp(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_STRAFE_LEFT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCStrafeRightDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, PLAYER_STRAFE_RIGHT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCStrafeRightUp(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, PLAYER_STRAFE_RIGHT);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCCameraUpDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, CAMERA_MOVE_UP);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCCameraUpUp(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, CAMERA_MOVE_UP);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCCameraDownDown(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerSetMovement(game, CAMERA_MOVE_DOWN);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyDCCameraDownUp(
	KEY_HANDLER_PARAMS)
{
	if (!game || !GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	c_PlayerClearMovement(game, CAMERA_MOVE_DOWN);
	return TRUE;
}
#endif // DEVELOPMENT_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sCannonLeaveUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

    c_SkillControlUnitRequestStartSkill( game, GlobalIndexGet( GI_SKILLS_CANNON_STOP_USING ) );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sActiveBarDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if (IsUnitDeadOrDying(unit))
	{
		return FALSE;
	}

	// we only want to do this the first time the key is pressed
	if (lParam & (1 << 30))
	{
		return FALSE;
	}

	const KEY_COMMAND & tKey = InputGetKeyCommand(command);

	int nTag = tKey.dwParam;
	if( AppIsTugboat() )
	{
		UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag( GameGetControlUnit( game ), nTag);
		
		if (tag && 
		(tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID))
		{

			int nSkillID = ( tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1] );

			if (nSkillID != INVALID_ID  )
			{
				
				sgbSpellHotkeyDown = TRUE;
				sInputHandleRightButtonSkill( game, TRUE, nSkillID );
				return TRUE;
			}
		}
		VECTOR vPlayerClickLocation = sGetPlayerClickLocation( game );
		UIActiveBarHotkeyDown(game, unit, nTag, &vPlayerClickLocation );		
	}
	else
	{
		UIActiveBarHotkeyDown(game, unit, nTag);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sActiveBarUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if (IsUnitDeadOrDying(unit))
	{
		return FALSE;
	}

	const KEY_COMMAND & tKey = InputGetKeyCommand(command);

	int nTag = tKey.dwParam;
	if( AppIsTugboat() )
	{
		UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag( GameGetControlUnit( game ), nTag);

		if (tag && 
		(tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID))
		{

			int nSkillID = ( tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1] );

			if (nSkillID != INVALID_ID  )
			{
				sInputHandleRightButtonSkill( game, FALSE, nSkillID );
				sgbSpellHotkeyDown = FALSE;
				return TRUE;

			}
		
		}
	}
	UIActiveBarHotkeyUp(game, unit, nTag);
	return TRUE;
}



//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
BOOL sHotSpellDown(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}
	ASSERT_RETFALSE(game && unit);
	if (IsUnitDeadOrDying(unit))
	{
		return FALSE;
	}

	// we only want to do this the first time the key is pressed
	if (lParam & (1 << 30))
	{
		return FALSE;
	}
	
	int nActiveSlot = command - CMD_HOTSPELL_1 + TAG_SELECTOR_SPELLHOTKEY1;
	const GLOBAL_DEFINITION * globals = DefinitionGetGlobal();
	ASSERT(globals);
	if( globals && (globals->dwFlags & GLOBAL_FLAG_CAST_ON_HOTKEY) )
	{

		UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag( GameGetControlUnit( game ), nActiveSlot);

		if (tag && 
		(tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID))
		{

			int nSkillID = ( tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1] );

			if (nSkillID != INVALID_ID  )
			{
				
				if( !UI_TB_MouseOverPanel() )
				{
					sgbSpellHotkeyDown = TRUE;
					sInputHandleRightButtonSkill( game, TRUE, nSkillID );
				}
			}
		
		}

	}

	UI_TB_HotSpellHotkeyDown(game, unit, nActiveSlot);

	
	return TRUE;
}

//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
BOOL sHotSpellUp(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}

	ASSERT_RETFALSE(game && unit);
	if (IsUnitDeadOrDying(unit))
	{
		return FALSE;
	}

	int nActiveSlot = command - CMD_HOTSPELL_1 + TAG_SELECTOR_SPELLHOTKEY1;
	UI_TB_HotSpellHotkeyUp(game, unit, nActiveSlot);
	const GLOBAL_DEFINITION * globals = DefinitionGetGlobal();
	ASSERT(globals);

	if( globals && (globals->dwFlags & GLOBAL_FLAG_CAST_ON_HOTKEY) )
	{

		UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag( GameGetControlUnit( game ), nActiveSlot);

		if (tag && 
		(tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID))
		{

			int nSkillID = ( tag->m_nSkillID[0] != INVALID_ID ? tag->m_nSkillID[0] : tag->m_nSkillID[1] );

			if (nSkillID != INVALID_ID  )
			{
				
				if( !UI_TB_MouseOverPanel() )
				{
					sInputHandleRightButtonSkill( game, FALSE, nSkillID );
				}
			}
		
		}

	}
	
	sgbSpellHotkeyDown = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sWeaponConfigDown(
	KEY_HANDLER_PARAMS)
{
	// TRAVIS: FIXME - temporarily disabled hotkeys
	if( AppIsTugboat() )
	{
		return FALSE;
	}
	ASSERT_RETFALSE(game && unit);
	if (IsUnitDeadOrDying(unit))
	{
		return FALSE;
	}

	MSG_CCMD_SELECT_WEAPONCONFIG msg;
	int nIndex = command - CMD_WEAPONCONFIG_1;

	if (!WeaponConfigCanSwapTo(game, unit, nIndex))
	{
		int nSoundId = GlobalIndexGet( GI_SOUND_KLAXXON_GENERIC_NO );
		if (nSoundId != INVALID_LINK)
		{
			c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
		}
		return TRUE;
	}

	msg.bHotkey = (BYTE)(nIndex + TAG_SELECTOR_WEAPCONFIG_HOTKEY1);

	c_SendMessage(CCMD_SELECT_WEAPONCONFIG, &msg);

	int nSoundId = GlobalIndexGet((GLOBAL_INDEX)(GI_SOUND_WEAPONCONFIG_1 + nIndex));
	if (nSoundId != INVALID_LINK)
	{
		c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyRestartUp(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	if (!IsUnitDeadOrDying(unit))
	{
		return FALSE;
	}
	
	if (AppIsHellgate())
	{
		return FALSE;
	}
	
	c_PlayerSendRespawn( PLAYER_RESPAWN_INVALID );	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyConsoleToggleChar(
	KEY_HANDLER_PARAMS)
{
	if (command != CMD_CHAT_ENTRY_TOGGLE_SLASH)
	{
		if( AppIsHellgate() )
		{
			UI_COMPONENT *pChat = UIComponentGetByEnum(UICOMP_CHAT_PANEL);
			if (pChat && UIComponentGetActive(pChat))
			{
				extern UI_MSG_RETVAL UIChatFadeIn( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
				UIChatFadeIn(pChat, NULL, 0, 0);
			}
		}
		UIChatSetContextBasedOnTab();
	}
	return ConsoleActivateEdit(command == CMD_CHAT_ENTRY_TOGGLE_SLASH, command != CMD_CHAT_ENTRY_TOGGLE_SLASH);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyConsoleToggle(
	KEY_HANDLER_PARAMS)
{

		ConsoleToggle();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyQuitDown(
	KEY_HANDLER_PARAMS)
{
	if( UI_TB_ModalDialogsOpen() )
	{
		UI_TB_HideAllMenus();
		return TRUE;
	}


	if ( AppIsHellgate() && game )
	{
		UNIT * unit = GameGetControlUnit(game);
		if (unit)
		{
			if (UnitTestFlag(unit, UNITFLAG_QUESTSTORY))
			{
				c_QuestAbortCurrent(game, unit);
				return TRUE;
			}
		}
	}
	AppPause(FALSE);
	AppUserPause(FALSE);
	c_SendRemovePlayer();
	return TRUE;
}

//----------------------------------------------------------------------------
// tugboat
//----------------------------------------------------------------------------
BOOL sKeyHideAll(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}
	UI_TB_HideAllMenus();

	return TRUE;
}

//----------------------------------------------------------------------------
// tugboat
//----------------------------------------------------------------------------
BOOL sKeyEngagePVPDown(
	KEY_HANDLER_PARAMS)
{

	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyGameMenuUp(
	KEY_HANDLER_PARAMS)
{
	if( AppIsTugboat() )
	{
		BOOL bClosedPanel = UI_TB_HideAllMenus();
		if( UI_TB_ModalDialogsOpen() )
		{
			bClosedPanel = TRUE;
			UI_TB_HideModalDialogs();			
		}
		if( bClosedPanel )
		{
			return TRUE;
		}
		
	}
	if (AppGetState() == APP_STATE_MAINMENU || AppGetState() == APP_STATE_CHARACTER_CREATE)
		return FALSE;

	if ( AppIsHellgate() && game)
	{
		UNIT * unit = GameGetControlUnit( game );
		if (unit)
		{
			if ( UnitTestFlag(unit, UNITFLAG_QUESTSTORY))
			{
				c_QuestAbortCurrent(game, unit);
				return TRUE;
			}
		}
	}
	
	if( AppIsTugboat() )
	{
		UIStopSkills();
	}
	else
	{
		UIStopAllSkillRequests();
	}

	c_PlayerClearAllMovement(AppGetCltGame());

	UIShowGameMenu();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyConsoleDown(
	KEY_HANDLER_PARAMS)
{
	ConsoleHandleInputKeyDown(game, wKey, lParam);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyConsoleChar(
	KEY_HANDLER_PARAMS)
{
	ConsoleHandleInputChar(game, wKey, lParam);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyConsoleExit(
	KEY_HANDLER_PARAMS)
{
	ConsoleSetEditActive(FALSE);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyHideInvDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	c_PlayerClearAllMovement( game );
	//DWORD dwFlags = 0;
	//if (command == CMD_UI_QUICK_EXIT)
	//{
	//	dwFlags = HIF_ESCAPE_OUT;
	//}
	//UIHideInventoryScreen(dwFlags);

	UIComponentActivate(UICOMP_INVENTORY, FALSE);

	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCancelTradeDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	BOOL bCancelled = (command == CMD_UI_QUICK_EXIT);
	UIShowTradeScreen( INVALID_ID, bCancelled, NULL );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyHideSkillMap(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	UIComponentActivate(UICOMP_SKILLMAP, FALSE);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCloseTownPortal(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	c_TownPortalUIClose(TRUE);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCloseNPCDialog(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);

	UIHideNPCDialog(CCT_CANCEL);
	UIHideNPCVideo();

	// call the npc dialog cancel callback
	UI_COMPONENT *pDialog =  UIComponentGetByEnum(UICOMP_NPC_CONVERSATION_DIALOG);
	const DIALOG_CALLBACK &tCallback = pDialog->m_tCallbackCancel;
	if (tCallback.pfnCallback)
	{
		tCallback.pfnCallback(tCallback.pCallbackData, tCallback.dwCallbackData);
	}
		
	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyCloseTipDialog(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyGetVideo(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(game && unit);
	UIRetrieveNPCVideo( game );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyGameMenuReturnUp(
	KEY_HANDLER_PARAMS)
{
	AppPause(FALSE);
	AppUserPause(FALSE);
	UIHideGameMenu();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyGameMenuDoSelectedUp(
	KEY_HANDLER_PARAMS)
{
	UIMenuDoSelected();

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyGameMenuSelectMoveUp(
	KEY_HANDLER_PARAMS)
{
	UIMenuChangeSelect(command == CMD_GAME_MENU_UP ? -1 : 1);
	return TRUE;
}


//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
BOOL sKeyOptionsMenuReturnUp(
	KEY_HANDLER_PARAMS)
{
	if( AppIsHellgate() )
	{
		return FALSE;
	}
	UIOptionsDialogCancel();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputBeginKeyRemap(
	int nCommand,
	int nKeySlot)
{
	ASSERT_RETURN(nCommand >= 0 && nCommand < NUM_KEY_COMMANDS);
	ASSERT_RETURN(nKeySlot >= 0 && nKeySlot < NUM_KEYS_PER_COMMAND);

	gInput.m_nKeyCommandBeingRemapped = nCommand;
	gInput.m_nKeyAssignBeingRemapped = nKeySlot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputEndKeyRemap(
	void)
{
	gInput.m_nKeyCommandBeingRemapped = -1;
	gInput.m_nKeyAssignBeingRemapped = -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyRemapKeyCancelDown( 
	KEY_HANDLER_PARAMS)
{
	InputEndKeyRemap();

	UISendMessage(WM_PAINT, 0, 0);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyClearMapKeyCancelDown( 
	KEY_HANDLER_PARAMS)
{
	g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].KeyAssign[gInput.m_nKeyAssignBeingRemapped].nKey = 0;
	g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].KeyAssign[gInput.m_nKeyAssignBeingRemapped].nModifierKey = 0;

	InputEndKeyRemap();

	UISendMessage(WM_PAINT, 0, 0);
	return TRUE;
}


////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void sKeyRemapConflictCallbackOk(
//	void *pCallbackData, 
//	DWORD dwCallbackData)
//{
//	ASSERT_RETURN(pCallbackData);
//
//	KEY_ASSIGNMENT *pKeyAssign = (KEY_ASSIGNMENT *)pCallbackData;
//
//	g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].KeyAssign[gInput.m_nKeyAssignBeingRemapped].nKey = pKeyAssign->nKey;
//	g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].KeyAssign[gInput.m_nKeyAssignBeingRemapped].nModifierKey = pKeyAssign->nModifierKey;
//
//	pKeyAssign->nKey = 0;
//	pKeyAssign->nModifierKey = 0;
//
//	InputEndKeyRemap();
//	UISendMessage(WM_PAINT, 0, 0);
//
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void sKeyRemapConflictCallbackCancel(
//	void *, 
//	DWORD )
//{
//	InputEndKeyRemap();
//	UISendMessage(WM_PAINT, 0, 0);
//}
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sKeyRemapKeyDown(
	KEY_HANDLER_PARAMS)
{
	ASSERT_RETFALSE(gInput.m_nKeyCommandBeingRemapped >= 0 && gInput.m_nKeyCommandBeingRemapped < NUM_KEY_COMMANDS);
	ASSERT_RETFALSE(gInput.m_nKeyAssignBeingRemapped >= 0 && gInput.m_nKeyAssignBeingRemapped < NUM_KEYS_PER_COMMAND);

	if (wKey == VK_ESCAPE) return FALSE;

	UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_OPTIONSDIALOG);
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	if (wKey == VK_SHIFT && UIButtonGetDown(pDialog, "shift as modifier btn")) return FALSE;
	if (wKey == VK_CONTROL && UIButtonGetDown(pDialog, "ctrl as modifier btn")) return FALSE;
	if (wKey == VK_MENU && UIButtonGetDown(pDialog, "alt as modifier btn")) return FALSE;

	int nModifierKey = 0;
	int iNumModifiers = 0;
	if (GetKeyState(VK_MENU) & 0x8000)		{++iNumModifiers; nModifierKey = VK_MENU;}
	if (GetKeyState(VK_SHIFT) & 0x8000)		{++iNumModifiers; nModifierKey = VK_SHIFT;}
	if (GetKeyState(VK_CONTROL) & 0x8000)	{++iNumModifiers; nModifierKey = VK_CONTROL;}
	if (iNumModifiers > 1)
	{
		// we don't allow multiple modifiers
		// this also has the desired side-effect of ignoring the AltGr key since it won't work correctly anyway
		return FALSE;
	}

	// special case
	if (wKey == VK_TAB && nModifierKey == VK_MENU)
		// ingore alt+tab
		return FALSE;

	UIOptionsRemapKey(gInput.m_nKeyCommandBeingRemapped,
		gInput.m_nKeyAssignBeingRemapped, 
		wKey,
		nModifierKey);

	//if (g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].bCheckConflicts)
	//{
	//	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	//	{
	//		KEY_COMMAND& key = g_pKeyAssignments[i];
	//		if (key.bCheckConflicts &&
	//			(key.eGame == AppGameGet() || key.eGame == APP_GAME_ALL))
	//		{
	//			for (int j=0; j < NUM_KEYS_PER_COMMAND; j++)
	//			{
	//				if (key.KeyAssign[j].nKey == (int)wKey &&
	//					key.KeyAssign[j].nModifierKey == nModifierKey)
	//				{
	//					if (key.bCheckConflicts &&
	//						!key.bRemappable)
	//					{
	//						UIShowGenericDialog(GlobalStringGet(GS_KEY_REMAP_CONFLICT_HEADER), GlobalStringGet(GS_KEY_REMAP_CONFLICT_UNAVAILABLE) );
	//					}
	//					else
	//					{
	//						WCHAR szBuf[512];
	//						PStrPrintf(szBuf, 512, GlobalStringGet(GS_KEY_REMAP_CONFLICT_MESSAGE), StringTableGetStringByIndex(key.nStringIndex));

	//						DIALOG_CALLBACK tOkCallback;
	//						DIALOG_CALLBACK tCancelCallback;
	//						DialogCallbackInit( tOkCallback );
	//						DialogCallbackInit( tCancelCallback );
	//						tOkCallback.pfnCallback = sKeyRemapConflictCallbackOk;
	//						tOkCallback.pCallbackData = &(key.KeyAssign[j]);
	//						tCancelCallback.pfnCallback = sKeyRemapConflictCallbackCancel;
	//						UIShowGenericDialog(GlobalStringGet(GS_KEY_REMAP_CONFLICT_HEADER), szBuf, TRUE, &tOkCallback, &tCancelCallback );
	//					}
	//					return TRUE;
	//				}
	//			}
	//		}
	//	}
	//}

	//g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].KeyAssign[gInput.m_nKeyAssignBeingRemapped].nKey = (DWORD)wKey;
	//g_pKeyAssignments[gInput.m_nKeyCommandBeingRemapped].KeyAssign[gInput.m_nKeyAssignBeingRemapped].nModifierKey = nModifierKey;

	//InputEndKeyRemap();
	return TRUE;
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL UIComponentIsActive(int n)
{
	return UIComponentGetActiveByEnum((UI_COMPONENT_ENUM) n);
}

inline BOOL IsActiveAlways(int)
{
	return TRUE;
}

inline BOOL InDemoMode(int)
{
	return ( NULL != DemoLevelGetDefinition() );
};

inline BOOL InScreenshotMode(int)
{
	return TRUE;
};

inline BOOL ConsoleIsEditActive(int)
{
	return ConsoleIsEditActive();
}

inline BOOL IsConsoleAvailable(int)
{
	return !ConsoleIsEditActive() && AppGetCltGame() && AppGetState() == APP_STATE_IN_GAME;
}

inline BOOL IsInAppState(int n)
{
	if (n == APP_STATE_IN_GAME && !AppGetCltGame())
	{
		return FALSE;
	}
	return AppGetState() == n;
}

inline BOOL IsDetatchedCameraActive(int)
{
	return AppGetDetachedCamera() && AppGetCltGame() && AppGetState() == APP_STATE_IN_GAME;
}

inline BOOL InventoryIsActive(int)
{
	return (!UIComponentGetActiveByEnum(UICOMP_SKILLMAP) &&
		UIComponentGetActiveByEnum(UICOMP_INVENTORY));
}

inline BOOL TradeIsActive(int)
{
	return UIComponentGetActiveByEnum( UICOMP_TRADE_PANEL ) ||
		   UIComponentGetActiveByEnum( UICOMP_REWARD_PANEL ) ||
		   ( AppIsTugboat() && UIPaneVisible( KPaneMenuTrade ) );
}

inline BOOL IsGameDebugFlag(int nDebugFlag)
{
	return GameGetDebugFlag(AppGetCltGame(), nDebugFlag);
}

inline BOOL IsRemappingKey(int)
{
	return (gInput.m_nKeyCommandBeingRemapped >= 0 && 
			gInput.m_nKeyCommandBeingRemapped < NUM_KEY_COMMANDS &&
			gInput.m_nKeyAssignBeingRemapped >= 0 && 
			gInput.m_nKeyAssignBeingRemapped < NUM_KEYS_PER_COMMAND);
}

BOOL InputIsRemappingKey()
{
	return IsRemappingKey(0);
}

inline BOOL PlayerHasState(int nState)
{
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit(pGame);
		if (pPlayer)
		{
			return UnitHasState( pGame, pPlayer, nState);
		}
	}
	return FALSE;
}

inline BOOL IsPlayerDead(int)
{
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit(pGame);
		if (pPlayer)
		{
			//return UnitTestMode(pPlayer, MODE_DEAD) || UnitTestMode(pPlayer, MODE_DYING);
			return IsUnitDeadOrDying(pPlayer);
		}
	}
	return FALSE;
}

inline BOOL IsInMovie(int)
{
	return AppGetState() == APP_STATE_PLAYMOVIELIST;
}

BOOL PauseMovie(
	KEY_HANDLER_PARAMS)
{
	if(c_MoviePlayerCanPause())
	{
		c_MoviePlayerUserControlTogglePaused();
	}
	return TRUE;
}

BOOL StopMovie(
	KEY_HANDLER_PARAMS)
{
	c_MoviePlayerSkipToEnd();
	return TRUE;
}

BOOL sEatKey(
	KEY_HANDLER_PARAMS)
{
	return TRUE;
}

BOOL SendKeyDownToUI(
	KEY_HANDLER_PARAMS)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum((UI_COMPONENT_ENUM)data);
	return ResultStopsProcessing(UIComponentHandleUIMessage(pComponent, UIMSG_KEYDOWN, wKey, lParam, TRUE));
}

BOOL SendKeyUpToUI(
	KEY_HANDLER_PARAMS)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum((UI_COMPONENT_ENUM)data);
	return ResultStopsProcessing(UIComponentHandleUIMessage(pComponent, UIMSG_KEYUP, wKey, lParam, TRUE));
}

BOOL SendKeyCharToUI(
	KEY_HANDLER_PARAMS)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum((UI_COMPONENT_ENUM)data);
	return ResultStopsProcessing(UIComponentHandleUIMessage(pComponent, UIMSG_KEYCHAR, wKey, lParam, TRUE));
}

BOOL OpenComponent(
	KEY_HANDLER_PARAMS)
{
	if (!UIComponentGetActiveByEnum((UI_COMPONENT_ENUM)data))
	{
		UIComponentActivate((UI_COMPONENT_ENUM)data, TRUE);
		return TRUE;
	}
	return FALSE;
}

BOOL CloseComponent(
	KEY_HANDLER_PARAMS)
{
	if (UIComponentGetActiveByEnum((UI_COMPONENT_ENUM)data))
	{
		UIComponentActivate((UI_COMPONENT_ENUM)data, FALSE);
		return TRUE;
	}
	return FALSE;
	//UI_COMPONENT *pComponent = UIComponentGetByEnum((UI_COMPONENT_ENUM)data);
	//return ResultStopsProcessing(UIComponentHandleUIMessage(pComponent, UIMSG_INACTIVATE, wKey, lParam, TRUE));
}

#if ISVERSION(DEVELOPMENT)
BOOL SendKeyDownToUIDebugEdit(
	KEY_HANDLER_PARAMS)
{
	return UIDebugEditKeyDown(wKey);
}
BOOL SendKeyUpToUIDebugEdit(
	KEY_HANDLER_PARAMS)
{
	return UIDebugEditKeyUp(wKey);
}
#endif

#if ISVERSION(DEVELOPMENT)
#define DECLARE_KEY_HANDLER(cmd, gm, unit, dwn, up, chr, data, app)	{cmd, gm, unit, dwn, up, chr, data, app, L#cmd, L#dwn, L#up, L#chr},	
#else
#define DECLARE_KEY_HANDLER(cmd, gm, unit, dwn, up, chr, data, app)	{cmd, gm, unit, dwn, up, chr, data, app},	
#endif

#if ISVERSION(DEVELOPMENT)
#define DECLARE_KEY_HANDLER_NO_RPT(cmd, gm, unit, dwn, up, chr, data, app)	{cmd, gm, unit, dwn, up, chr, data, app, L#cmd, L#dwn, L#up, L#chr, TRUE},	
#else
#define DECLARE_KEY_HANDLER_NO_RPT(cmd, gm, unit, dwn, up, chr, data, app)	{cmd, gm, unit, dwn, up, chr, data, app, TRUE},	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGameMovementKeyHandlerTbl[] =
{	// command								game?	unit?	down							up							char  data  app
	DECLARE_KEY_HANDLER( CMD_TURN_LEFT,		TRUE,	TRUE,	sKeyStrRotLeftDown,				sKeyStrRotLeftUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_TURN_RIGHT,	TRUE,	TRUE,	sKeyStrRotRightDown,			sKeyStrRotRightUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_MOVE_FORWARD,	TRUE,	TRUE,	sKeyForwardDown,				sKeyForwardUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_MOVE_BACK,		TRUE,	TRUE,	sKeyBackDown,					sKeyBackUp,					NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_MOVE_LEFT,		TRUE,	TRUE,	sKeyStrafeLeftDown,				sKeyStrafeLeftUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_MOVE_RIGHT,	TRUE,	TRUE,	sKeyStrafeRightDown,			sKeyStrafeRightUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_AUTORUN,		TRUE,	TRUE,	sKeyAutorun,					NULL,						NULL, 0,	APP_GAME_ALL )

	DECLARE_KEY_HANDLER( CMD_ZOOM_IN,		TRUE,	TRUE,	sKeyCameraZoom,					NULL,						NULL, 1,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_ZOOM_OUT,		TRUE,	TRUE,	sKeyCameraZoom,					NULL,						NULL, (DWORD_PTR)-1,	APP_GAME_HELLGATE )

	//DECLARE_KEY_HANDLER( CMD_CROUCH,		TRUE,	TRUE,	sKeyCrouchDown,					sKeyCrouchUp,				NULL, 0,	APP_GAME_ALL )
	{					 -1,				FALSE,	FALSE,	NULL,							NULL,						NULL, 0,	APP_GAME_ALL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gJumpMovementKeyHandlerTbl[] =				// this needs to be in a separate table so it doesn't get processed while you're in an inventory screen
{	// command								game?	unit?	down							up							char
	DECLARE_KEY_HANDLER( CMD_JUMP,			TRUE,	TRUE,	sKeyJumpDown,					sKeyJumpUp,					NULL, 0,	APP_GAME_ALL )
	{					 -1,				FALSE,	FALSE,	NULL,							NULL,						NULL, 0,	APP_GAME_ALL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGameControlKeyHandlerTbl[] =
{	// command									game?	unit?	down						up							char  data  app	
//	DECLARE_KEY_HANDLER( CMD_AUTO_PARTY,		TRUE,	TRUE,	NULL,						sKeyAutoParty,				NULL, 0,	APP_GAME_ALL )	
	
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_WORLD_MAP,	TRUE,	TRUE,	sKeyWorldMap,				NULL,						NULL, 0,	APP_GAME_ALL)
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_MAP,			TRUE,	TRUE,	sKeyAutomap,				NULL,						NULL, 0,	APP_GAME_ALL )

	DECLARE_KEY_HANDLER_NO_RPT( CMD_WEAPON_SWAP,		TRUE,	TRUE,	sKeyWeaponSetSwap,			NULL,						NULL, 0,	APP_GAME_TUGBOAT)

#if ISVERSION(DEVELOPMENT)
	DECLARE_KEY_HANDLER( CMD_OPEN_FULLLOG,		TRUE,	TRUE,	sKeyOpenFullLog,			NULL,						NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_CLOSE_FULLLOG,		TRUE,	TRUE,	sKeyCloseFullLog,			NULL,						NULL, 0,	APP_GAME_ALL )
#endif

	DECLARE_KEY_HANDLER( CMD_MAP_ZOOM_IN,		TRUE,	TRUE,	sKeyAutoMapZoomIn,					NULL,						NULL, (DWORD_PTR)-1,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_MAP_ZOOM_OUT,		TRUE,	TRUE,	sKeyAutoMapZoomOut,					NULL,						NULL, 1,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_CAMERAORBIT,		TRUE,	TRUE,	sKeyCameraOrbitDown,				sKeyCameraOrbitUp,						NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_RESETCAMERAORBIT,	TRUE,	TRUE,	sKeyResetCameraOrbit,				NULL,						NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_AUTORUN,			TRUE,	TRUE,	sKeyAutorun,					NULL,						NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_FORCEMOVE,			TRUE,	TRUE,	sKeyForceMove,					NULL,						NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ZOOM_IN,			TRUE,	TRUE,	sKeyCameraZoom,					NULL,						NULL, 1,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ZOOM_OUT,			TRUE,	TRUE,	sKeyCameraZoom,					NULL,						NULL, (DWORD_PTR)-1,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_CAMERA_FORWARD,	TRUE,	TRUE,	sKeyForwardDown,				sKeyForwardUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_CAMERA_BACK,		TRUE,	TRUE,	sKeyBackDown,					sKeyBackUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_CAMERA_LEFT,		TRUE,	TRUE,	sKeyStrafeLeftDown,				sKeyStrafeLeftUp,			NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_CAMERA_RIGHT,		TRUE,	TRUE,	sKeyStrafeRightDown,			sKeyStrafeRightUp,			NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_CAMERA_ORBITLEFT,	TRUE,	TRUE,	sKeyStrRotLeftDown,				sKeyStrRotLeftUp,			NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_CAMERA_ORBITRIGHT,	TRUE,	TRUE,	sKeyStrRotRightDown,			sKeyStrRotRightUp,			NULL, 0,	APP_GAME_TUGBOAT )

	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_BUDDYLIST,	TRUE,	TRUE,	NULL,						sKeyBuddyList,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_FRIENDLIST,	TRUE,	TRUE,	NULL,						sKeyBuddyList,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_GUILDPANEL,	TRUE,	TRUE,	NULL,						sKeyGuildPanel,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_PARTYPANEL,	TRUE,	TRUE,	NULL,						sKeyPartyPanel,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_OPEN_EMAIL,		TRUE,	TRUE,	NULL,						sKeyEmail,					NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_EMAIL_TUGBOAT,TRUE,	TRUE,	sKeyEmail,					NULL,						NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_SHOW_ITEMS,	TRUE,	TRUE,	sKeyShowItemsDown,			sKeyShowItemsUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ITEM_PICKUP,		TRUE,	TRUE,	sKeyItemPickup,				NULL,						NULL, 0,	APP_GAME_HELLGATE )

	DECLARE_KEY_HANDLER( CMD_SKILL_SHIFT,		TRUE,	TRUE,	sKeySkillShiftDown,			sKeySkillShiftUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_SKILL_POTION,		TRUE,	TRUE,	sKeySkillPotionDown,		sKeySkillPotionUp,			NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU,			TRUE,	TRUE,	NULL,						sKeyGameMenuUp,				NULL, 0,	APP_GAME_ALL )

	DECLARE_KEY_HANDLER( CMD_HIDE_ALL,			TRUE,	TRUE,	sKeyHideAll,				NULL,						NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ENGAGE_PVP,		TRUE,	TRUE,	sKeyEngagePVPDown,			NULL,			NULL, 0,	APP_GAME_TUGBOAT )
	
#if ISVERSION(DEVELOPMENT) && (!ISVERSION(DEMO_VERSION))
	DECLARE_KEY_HANDLER( CMD_EXIT,				TRUE,	TRUE,	sKeyQuitDown,				NULL,						NULL, 0,	APP_GAME_ALL )
#endif

#if ISVERSION(DEVELOPMENT)
	DECLARE_KEY_HANDLER( CMD_OPEN_AUCTION,		TRUE,	TRUE,	NULL,						sKeyAuctionHouse,			NULL, 0,	APP_GAME_HELLGATE )
#endif
	{					 -1,					FALSE,	FALSE,	NULL,						NULL,						NULL, 0,	APP_GAME_ALL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gTugboatAutomapHandler[] =
{	// command									game?	unit?	down						up							char  data  app	
	DECLARE_KEY_HANDLER( CMD_MYTHOS_MAP_ZOOM_IN,	TRUE,	TRUE,	sKeyAutoMapZoomIn,			NULL,						NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_MYTHOS_MAP_ZOOM_OUT,	TRUE,	TRUE,	sKeyAutoMapZoomOut,			NULL,						NULL, 0,	APP_GAME_ALL )
	{					 -1,					FALSE,	FALSE,	NULL,						NULL,						NULL, 0,	APP_GAME_ALL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gCameraControlKeyHandlerTbl[] =
{	// command										game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_VANITY_MODE,			TRUE,	TRUE,	sKeyVanityCamDown,		sKeyVanityCamUp,			NULL, 0,	APP_GAME_ALL )
#if (!ISVERSION(DEMO_VERSION) && ISVERSION(DEVELOPMENT))
	DECLARE_KEY_HANDLER( CMD_CAM_SINGLE_STEP,		FALSE,	FALSE,	sKeySingleStepDown,		NULL,						NULL, 0,	APP_GAME_ALL )	// '\'
	DECLARE_KEY_HANDLER( CMD_CAM_RESUME_SPEED,		FALSE,	FALSE,	sKeyResumeSpeedDown,	NULL,						NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_CAM_SLOWER_SPEED,		FALSE,	FALSE,	sKeySlowerSpeedDown,	NULL,						NULL, 0,	APP_GAME_HELLGATE )	// '['
	DECLARE_KEY_HANDLER( CMD_CAM_FASTER_SPEED,		FALSE,	FALSE,	sKeyFasterSpeedDown,	NULL,						NULL, 0,	APP_GAME_HELLGATE )	// ']'
	DECLARE_KEY_HANDLER( CMD_TUG_CAM_SLOWER_SPEED,	FALSE,	FALSE,	sKeySlowerSpeedDown,	NULL,						NULL, 0,	APP_GAME_TUGBOAT )	// CTRL '['
	DECLARE_KEY_HANDLER( CMD_TUG_CAM_FASTER_SPEED,	FALSE,	FALSE,	sKeyFasterSpeedDown,	NULL,						NULL, 0,	APP_GAME_TUGBOAT )	// CTRL ']'
	DECLARE_KEY_HANDLER( CMD_CAM_DETATCH,			FALSE,	FALSE,	sKeyDetachCamera,		NULL,						NULL, 0,	APP_GAME_ALL )	// ';'
	DECLARE_KEY_HANDLER( CMD_SIMULATE_BAD_FPS,		FALSE,	FALSE,	sKeySimulateBadFPS,		NULL,						NULL, 0,					APP_GAME_HELLGATE )	// ']'
#endif
	{					 -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
KEY_HANDLER gDetachedCameraKeyHandlerTbl[] =
{	// command										game?	unit?	down					up							char
#if (!ISVERSION(DEMO_VERSION))
	DECLARE_KEY_HANDLER( CMD_DET_CAM_STRAFE_LEFT,	TRUE,	TRUE,	sKeyDCStrafeLeftDown,	sKeyDCStrafeLeftUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DET_CAM_BACK,			TRUE,	TRUE,	sKeyDCBackDown,			sKeyDCBackUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DET_CAM_FORWARD,		TRUE,	TRUE,	sKeyDCForwardDown,		sKeyDCForwardUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DET_CAM_STRAFE_RIGHT,	TRUE,	TRUE,	sKeyDCStrafeRightDown,	sKeyDCStrafeRightUp,		NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DET_CAM_UP,			TRUE,	TRUE,	sKeyDCCameraUpDown,		sKeyDCCameraUpUp,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DET_CAM_DOWN,			TRUE,	TRUE,	sKeyDCCameraDownDown,	sKeyDCCameraDownUp,			NULL, 0,	APP_GAME_ALL )
#endif
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gActiveBarKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_1,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_2,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_3,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_4,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_5,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_6,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_7,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_8,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_9,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_10,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_11,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_12,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_11_T,		TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_12_T,		TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_1CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_2CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_3CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_4CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_5CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_6CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_7CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_8CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_9CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_10CTRL,			TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_11_CTRL,		TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_12_CTRL,		TRUE,	TRUE,	sActiveBarDown,				sActiveBarUp,				NULL, 0,	APP_GAME_TUGBOAT )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gHotSpellKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_1,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_2,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_3,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_4,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_5,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_6,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_7,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_8,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_9,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_10,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_11,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_HOTSPELL_12,			TRUE,	TRUE,	sHotSpellDown,				sHotSpellUp,				NULL, 0,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL },
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gWeaponConfigKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_WEAPONCONFIG_1,		TRUE,	TRUE,	sWeaponConfigDown,			NULL,						NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_WEAPONCONFIG_2,		TRUE,	TRUE,	sWeaponConfigDown,			NULL,						NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_WEAPONCONFIG_3,		TRUE,	TRUE,	sWeaponConfigDown,			NULL,						NULL, 0,	APP_GAME_HELLGATE )

	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gRestartKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_RESTART_AFTER_DEATH,	TRUE,	TRUE,	NULL,						sKeyRestartUp,				NULL, 0,	APP_GAME_HELLGATE )

// since this table is blocking, we need to duplicate the game menu handler here
	DECLARE_KEY_HANDLER( CMD_GAME_MENU,				TRUE,	TRUE,	NULL,					sKeyGameMenuUp,				NULL, 0,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gChatEditToggleKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_CHAT_TOGGLE,			TRUE,	TRUE,	NULL,						sKeyChatToggle,				NULL,				   0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE_SLASH,	FALSE,	FALSE,	NULL,					NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE,		FALSE,	FALSE,	NULL,						NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )		

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gDemoModeKeyHandlerTbl[] =
{						// command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER(  CMD_GAME_MENU,			FALSE,	FALSE,	sDemoModeInterrupt,			NULL,						NULL, 0,	APP_GAME_ALL )
#if ISVERSION(DEVELOPMENT)
	//DECLARE_KEY_HANDLER(  CMD_SCREENSHOT_BURST,		FALSE,	FALSE,	sTakeScreenshotBurst,		NULL,						NULL, 0,	APP_GAME_ALL )
#endif
	{  -1,					FALSE,	FALSE,	NULL,	NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gScreenshotKeyHandlerTbl[] =
{						// command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER(  CMD_SCREENSHOT,			FALSE,	FALSE,	sTakeScreenshot,			NULL,						NULL, 0,	APP_GAME_ALL )
#if ISVERSION(DEVELOPMENT)
	//DECLARE_KEY_HANDLER(  CMD_SCREENSHOT_BURST,		FALSE,	FALSE,	sTakeScreenshotBurst,		NULL,						NULL, 0,	APP_GAME_ALL )
#endif
	{  -1,					FALSE,	FALSE,	NULL,	NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gEngineDebugKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_DEBUG_UP,				FALSE,	FALSE,	sDebugUp,					sScreenshotNOP,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DEBUG_DOWN,			FALSE,	FALSE,	sDebugDown,					sScreenshotNOP,				NULL, 0,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGameDebugKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
#if ISVERSION(DEVELOPMENT)
	DECLARE_KEY_HANDLER( CMD_SELECT_DEBUG_UNIT,		TRUE,	TRUE,	sKeySelectDebugUnit,		NULL,						NULL, 0,	APP_GAME_ALL )	// ','
	DECLARE_KEY_HANDLER( CMD_SELECT_DEBUG_SELF,		TRUE,	TRUE,	sKeySelectDebugSelf,		NULL,						NULL, 0,	APP_GAME_ALL )	// '.'

	DECLARE_KEY_HANDLER( CMD_DRB_DEBUG,				TRUE,	TRUE,	sKeyDRBDebug,				NULL,						NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_DRB_DEBUG2,			TRUE,	TRUE,	sKeyDRBDebug2,				NULL,						NULL, 0,	APP_GAME_ALL )
#endif
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gUIEditKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUIDebugEdit,	SendKeyUpToUIDebugEdit,		NULL, 0 ,	APP_GAME_ALL)	// ','
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gDebugKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_DEBUG_TEXT_LENGTH,		FALSE,	FALSE,	sKeyDebugTextLength,		NULL,						NULL, 0,	APP_GAME_ALL )	
	DECLARE_KEY_HANDLER( CMD_DEBUG_TEXT_LABELS,		FALSE,	FALSE,	sKeyDebugTextLabels,		NULL,						NULL, 0,	APP_GAME_ALL )		
	DECLARE_KEY_HANDLER( CMD_DEBUG_TEXT_DEVELOPER,	FALSE,	FALSE,	sKeyDebugTextDeveloper,		NULL,						NULL, 0,	APP_GAME_ALL )			
	DECLARE_KEY_HANDLER( CMD_DEBUG_TEXT_ENGLISH,	FALSE,	FALSE,	sKeyDebugTextDeveloper,		NULL,						NULL, 0,	APP_GAME_ALL )			
	DECLARE_KEY_HANDLER( CMD_DEBUG_UI_CLIPPING,		FALSE,	FALSE,	sKeyDebugUIClipping,		NULL,						NULL, 0,	APP_GAME_ALL )		
	DECLARE_KEY_HANDLER( CMD_DEBUG_FONT_POINT_SIZE,	FALSE,	FALSE,	sKeyDebugTextPointSize,		NULL,						NULL, 0,	APP_GAME_ALL )		

	DECLARE_KEY_HANDLER( CMD_DEBUG_UI_EDIT,			FALSE,	FALSE,	sKeyDebugUIEdit,			NULL,						NULL, 0,	APP_GAME_ALL )		
	DECLARE_KEY_HANDLER( CMD_DEBUG_UI_RELOAD,		FALSE,	FALSE,	sKeyDebugUIReload,			NULL,						NULL, 0,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gChatEditKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_EXIT,		FALSE,	FALSE,	NULL,						sKeyConsoleExit,			NULL, 			 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_PREV_CMD,	FALSE,	FALSE,	sKeyConsoleChar,			NULL,						NULL, 			 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_NEXT_CMD,	FALSE,	FALSE,	sKeyConsoleChar,			NULL,						NULL, 			 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	sKeyConsoleDown,			NULL,						sKeyConsoleChar, 0,	APP_GAME_ALL )
	{ -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gUIActivationKeyHandlerTbl[] =
{	// command											game?	unit?	down				up							char
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_INVENTORY,		TRUE,	TRUE,	NULL,				sKeyInventoryScreen,		NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_INVENTORY_TUGBOAT,		TRUE,	TRUE,	NULL,		sKeyInventoryScreen,		NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_SKILLS,		TRUE,	TRUE,	NULL,				sKeySkillMapScreen,			NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_PERKS,			TRUE,	TRUE,	NULL,				sKeyPerkMapScreen,			NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_QUESTLOG,		TRUE,	TRUE,	NULL,				sKeyQuestLog,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_ACHIEVEMENTS,	TRUE,	TRUE,	NULL,				sKeyAchievements,			NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_ACHIEVEMENTS_MYTHOS,	TRUE,	TRUE,	NULL,		sKeyAchievements,			NULL, 0,	APP_GAME_TUGBOAT )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_CRAFTING_MYTHOS,		TRUE,	TRUE,	NULL,		sKeyCrafting,			NULL, 0,	APP_GAME_TUGBOAT )	
	DECLARE_KEY_HANDLER_NO_RPT( CMD_UI_QUICK_EXIT,		TRUE,	TRUE,	NULL,				sKeyQuickClose,				NULL, 0,	APP_GAME_ALL )

	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_CHARACTER,		TRUE,	TRUE,	NULL,				sKeyCharacterScreen,		NULL, 0,	APP_GAME_TUGBOAT )
	//DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_EMAIL,			TRUE,	TRUE,	NULL,				sKeyEmail,					NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_EMAIL_TUGBOAT,			TRUE,	TRUE,	sKeyEmail, NULL,							NULL, 0,	APP_GAME_TUGBOAT )
	{							-1,						FALSE,	FALSE,	NULL,				NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gInventoryKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_QUEST_PANEL,		APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gTradeKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_TRADE_EDIT_MONEY_YOURS,	APP_GAME_ALL )	
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//---------------------------------------------------------------------------- 
KEY_HANDLER gSkillMapKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_QUEST_PANEL,		APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gQuestPanelKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_QUEST_PANEL,		APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,					NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gAchievementsPanelKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_ACHIEVEMENTS,		APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,					NULL }
};

//----------------------------------------------------------------------------
//---------------------------------------------------------------------------- 
KEY_HANDLER gSocialScreenKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_UI_ELEMENT_CLOSE,		TRUE,	TRUE,	sEatKey,					sKeyHideSocialScreen,		NULL,				0,						APP_GAME_ALL )
//	DECLARE_KEY_HANDLER( CMD_OPEN_CHAT_SCREEN,		TRUE,	TRUE,	sEatKey,					sKeyHideSocialScreen,		NULL,				0,						APP_GAME_ALL )
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_SOCIAL_SCREEN,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gTownPortalKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gNPCDialogKeyHandlerTbl[] =
{						// command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE_SLASH,	FALSE,	FALSE,	NULL,						NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_ELEMENT_CLOSE,		TRUE,	TRUE,	SendKeyDownToUI,			SendKeyUpToUI,				NULL, UICOMP_NPC_CONVERSATION_DIALOG,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_NPC_DIALOG_ACCEPT,		TRUE,	TRUE,	SendKeyDownToUI,			SendKeyUpToUI,				NULL, UICOMP_NPC_CONVERSATION_DIALOG,	APP_GAME_ALL)
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_NPC_CONVERSATION_DIALOG,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gNPCVideoDialogKeyHandlerTbl[] =
{						// command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE_SLASH,	FALSE,	FALSE,	NULL,						NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_ELEMENT_CLOSE,		TRUE,	TRUE,	SendKeyDownToUI,			SendKeyUpToUI,				NULL, UICOMP_VIDEO_DIALOG,		APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_NPC_DIALOG_ACCEPT,		TRUE,	TRUE,	SendKeyDownToUI,			SendKeyUpToUI,						NULL, UICOMP_VIDEO_DIALOG,		APP_GAME_ALL)
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gTipDialogKeyHandlerTbl[] =
{						// command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_UI_TIP_CLOSE,			TRUE,	TRUE,	NULL,						sKeyCloseTipDialog,			NULL, 0,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gVideoIncomingKeyHandlerTbl[] =
{						// command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_VIDEO_INCOMING,		TRUE,	TRUE,	sEatKey,					sKeyGetVideo,				sEatKey, 0,	APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGameMenuKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_RETURN,		FALSE,	FALSE,	NULL,					sKeyGameMenuReturnUp,		NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_DOSELECTED,	FALSE,	FALSE,	NULL,					sKeyGameMenuDoSelectedUp,	NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_UP,			FALSE,	FALSE,	NULL,					sKeyGameMenuSelectMoveUp,	NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_DOWN,		FALSE,	FALSE,	NULL,					sKeyGameMenuSelectMoveUp,	NULL, 0,	APP_GAME_ALL )
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
KEY_HANDLER gOptionsMenuKeyHandlerTbl[] =
{	//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_UI_ELEMENT_CLOSE,		TRUE,	TRUE,	NULL,					sKeyOptionsMenuReturnUp,	NULL, 0,	APP_GAME_ALL )
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gBugReportHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_BUG_REPORT,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gOptionsKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_OPTIONSDIALOG,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gKeyRemapKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_REMAP_KEY_CANCEL,		TRUE,	TRUE,	NULL,					sKeyRemapKeyCancelDown,		NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_REMAP_KEY_CLEAR,		TRUE,	TRUE,	NULL,					sKeyClearMapKeyCancelDown,	NULL, 0,	APP_GAME_ALL )
		
	DECLARE_KEY_HANDLER(  -2,						FALSE,	FALSE,	sKeyRemapKeyDown,			NULL,			NULL, 0,	APP_GAME_ALL )
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gStartMenuKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_STARTGAME_SCREEN,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_UP,			FALSE,	FALSE,	NULL,					sKeyGameMenuSelectMoveUp,	NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_DOWN,		FALSE,	FALSE,	NULL,					sKeyGameMenuSelectMoveUp,	NULL, 0,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gEulaScreenKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_EULA_SCREEN,	APP_GAME_ALL )
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gKioskKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_KIOSK,	APP_GAME_ALL )
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gBuddyListKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_ADDBUDDY,		APP_GAME_ALL )
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_SETNICKBUDDY,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyBuddyList,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_BUDDYLIST,		TRUE,	TRUE,	NULL,						sKeyBuddyList,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_FRIENDLIST,		TRUE,	TRUE,	NULL,						sKeyBuddyList,				NULL, 0,	APP_GAME_TUGBOAT )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGuildMgtKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_GUILDPANEL,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyQuickClose,				NULL, 0,			APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGuildInviteKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_GUILDPANEL,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyQuickClose,				NULL, 0,			APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gGuildPanelKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyQuickClose,				NULL, 0,			APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_GUILDPANEL,TRUE,	TRUE,	NULL,						sKeyGuildPanel,				NULL, 0,			APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gPartyPanelKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_PARTYPANEL,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyPartyPanel,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_PARTYPANEL,		TRUE,	TRUE,	NULL,						sKeyPartyPanel,				NULL, 0,	APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gPlayerMatchKeyHandlerTbl[] =
{						//command					game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_PLAYERMATCH_PANEL,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyPartyPanel,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER_NO_RPT( CMD_OPEN_PARTYPANEL,		TRUE,	TRUE,	NULL,						sKeyPartyPanel,				NULL, 0,	APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gEmailViewKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyQuickClose,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_UP,			TRUE,	TRUE,	NULL,						sKeyEmailScrollUp,			NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_DOWN,		TRUE,	TRUE,	NULL,						sKeyEmailScrollDown,		NULL, 0,	APP_GAME_HELLGATE )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gEmailComposeKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_GAME_MENU_RETURN,		TRUE,	TRUE,	NULL,						sKeyQuickClose,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_EMAIL_BODY_PANEL,		APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gAuctionKeyHandlerTbl[] =
{	// command										game?	unit?	down						up							char
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,						sKeyQuickClose,				NULL, 0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( -2,						FALSE,	FALSE,	SendKeyDownToUI,			SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_AUCTION_PANEL,		APP_GAME_ALL )
	{  -1,					FALSE,	FALSE,	NULL,						NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gCharacterCreateKeyHandlerTbl[] =
{						//command					game?	unit?	down					up							char
#if ISVERSION(DEVELOPMENT)
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE_SLASH,	FALSE,	FALSE,	NULL,					NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )
#endif
#if (!ISVERSION(DEMO_VERSION) && ISVERSION(DEVELOPMENT))
	DECLARE_KEY_HANDLER( CMD_CAM_SINGLE_STEP,			FALSE,	FALSE,	sKeySingleStepDown,		NULL,						NULL, 0,					APP_GAME_ALL )	// '\'
	DECLARE_KEY_HANDLER( CMD_CAM_RESUME_SPEED,			FALSE,	FALSE,	sKeyResumeSpeedDown,	NULL,						NULL, 0,					APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_CAM_SLOWER_SPEED,			FALSE,	FALSE,	sKeySlowerSpeedDown,	NULL,						NULL, 0,					APP_GAME_HELLGATE )	// '['
	DECLARE_KEY_HANDLER( CMD_CAM_FASTER_SPEED,			FALSE,	FALSE,	sKeyFasterSpeedDown,	NULL,						NULL, 0,					APP_GAME_HELLGATE )	// ']'
	DECLARE_KEY_HANDLER( CMD_TUG_CAM_SLOWER_SPEED,		FALSE,	FALSE,	sKeySlowerSpeedDown,	NULL,						NULL, 0,					APP_GAME_TUGBOAT )	// CTRL '['
	DECLARE_KEY_HANDLER( CMD_TUG_CAM_FASTER_SPEED,		FALSE,	FALSE,	sKeyFasterSpeedDown,	NULL,						NULL, 0,					APP_GAME_TUGBOAT )	// CTRL ']'
	DECLARE_KEY_HANDLER( CMD_CAM_DETATCH,				FALSE,	FALSE,	sKeyDetachCamera,		NULL,						NULL, 0,					APP_GAME_ALL )	// ';'
	DECLARE_KEY_HANDLER( CMD_SIMULATE_BAD_FPS,			FALSE,	FALSE,	sKeySimulateBadFPS,		NULL,						NULL, 0,					APP_GAME_HELLGATE )	// ']'
#endif
	DECLARE_KEY_HANDLER( CMD_ZOOM_IN,					FALSE,	FALSE,	sKeyCameraZoomCharacterCreate,			NULL,		NULL, 1,					APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER( CMD_ZOOM_OUT,					FALSE,	FALSE,	sKeyCameraZoomCharacterCreate,			NULL,		NULL, (DWORD_PTR)-1,		APP_GAME_HELLGATE )

	DECLARE_KEY_HANDLER(  -2,							FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI, UICOMP_CHARACTER_SELECT,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER(  -2,							FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI, UICOMP_CHARACTER_CREATE,	APP_GAME_ALL )
		
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gMovieHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER(  CMD_MOVIE_PAUSE,			FALSE,	FALSE,	NULL,					NULL,						PauseMovie,					0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER(  CMD_MOVIE_CANCEL,			FALSE,	FALSE,	NULL,					NULL,						StopMovie,					0,	APP_GAME_HELLGATE )
	DECLARE_KEY_HANDLER(  -2,						FALSE,	FALSE,	NULL,					NULL,						sEatKey,					0,	APP_GAME_HELLGATE )
		
	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gCannonModeHandlerTbl[] =
{						//command					game?	unit?	down				up							char
	DECLARE_KEY_HANDLER( CMD_UI_QUICK_EXIT,			TRUE,	TRUE,	NULL,				sCannonLeaveUp,				NULL, 0,	APP_GAME_HELLGATE )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
KEY_HANDLER gAltActivebarHandlerTbl[] =
{						//command					game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_13,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_14,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_15,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_16,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_17,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_18,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_19,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_20,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_21,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_22,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_23,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER( CMD_ACTIVESLOT_24,			TRUE,	TRUE,	sActiveBarDown,			sActiveBarUp,				NULL, 0,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};

KEY_HANDLER gRTSModeHandlerTbl[] =
{						//command					game?	unit?	down					up							char
// since this table is blocking, we need to duplicate the game menu handler here
	DECLARE_KEY_HANDLER( CMD_GAME_MENU,				TRUE,	TRUE,	NULL,					sKeyGameMenuUp,				NULL, 0,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL }
};


KEY_HANDLER gDialogKeyHandlerTbl[] =
{						  //command						game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE_SLASH,	FALSE,	FALSE,	NULL,					NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )

	DECLARE_KEY_HANDLER(  -2,							FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_GENERIC_DIALOG,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER(  CMD_GAME_MENU_DOSELECTED,		FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_GENERIC_DIALOG,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL },
};

KEY_HANDLER gGuildCreateKeyHandlerTbl[] =
{						  //command						game?	unit?	down					up							char
	DECLARE_KEY_HANDLER(  -2,							FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_GUILD_CREATE_DIALOG,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL },
};

KEY_HANDLER gStackSplitKeyHandlerTbl[] =
{						  //command						game?	unit?	down					up							char
	DECLARE_KEY_HANDLER(  -2,							FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_STACK_SPLIT_DIALOG,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL },
};

KEY_HANDLER gInviteKeyHandlerTbl[] =
{						  //command						game?	unit?	down					up							char
	DECLARE_KEY_HANDLER( CMD_CHAT_ENTRY_TOGGLE_SLASH,	FALSE,	FALSE,	NULL,					NULL,						sKeyConsoleToggleChar, 0,	APP_GAME_ALL )

	DECLARE_KEY_HANDLER(  -2,							FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_PARTY_INVITE_DIALOG,	APP_GAME_ALL )
	DECLARE_KEY_HANDLER(  CMD_GAME_MENU_DOSELECTED,		FALSE,	FALSE,	SendKeyDownToUI,		SendKeyUpToUI,				SendKeyCharToUI,	UICOMP_PARTY_INVITE_DIALOG,	APP_GAME_ALL )

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL },
};

KEY_HANDLER gConsoleToggleHandlerTbl[] =
{						  //command					game?	unit?	down					up							char
//#if ISVERSION(DEVELOPMENT)
	DECLARE_KEY_HANDLER( CMD_CONSOLE_TOGGLE,		FALSE,	FALSE,	sKeyConsoleToggle,		NULL,						NULL,				   0,	APP_GAME_ALL )
//#endif

	{  -1,						FALSE,	FALSE,	NULL,					NULL,						NULL },
};


typedef BOOL (*FP_HANDLER_TBL_ACTIVE)(int nParam);

#if ISVERSION(DEVELOPMENT)
#define DECLARE_HANDLER_TBL(tbl, blk, cht, fp, prm, app)	{L#tbl, tbl, blk, cht, fp, prm, app },
#else
#define DECLARE_HANDLER_TBL(tbl, blk, cht, fp, prm, app)	{tbl, blk, cht, fp, prm, app },
#endif

struct KEY_HANDLER_LIST_ENTRY{
#if ISVERSION(DEVELOPMENT)
	WCHAR					szDebugName[256];
#endif
	KEY_HANDLER *			pKeyHandlerTbl;
	BOOL					bBlocking;
	BOOL					bCheats;
	FP_HANDLER_TBL_ACTIVE	fpIsActive;
	int						nIsActiveParam;
	APP_GAME				eGame	;
} 
gKeyHandersList[] = 
{							//						  blocking  cheats	activefunc					activeparam					apptype
	DECLARE_HANDLER_TBL(gKeyRemapKeyHandlerTbl,			TRUE,	FALSE,	IsRemappingKey,				0,							APP_GAME_ALL	)

#if ISVERSION(DEVELOPMENT)					
	DECLARE_HANDLER_TBL(gUIEditKeyHandlerTbl,			FALSE,	TRUE,	IsGameDebugFlag,			DEBUGFLAG_UI_EDIT_MODE,		APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gDebugKeyHandlerTbl,			FALSE,	TRUE,  	IsActiveAlways,				0,							APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gGameDebugKeyHandlerTbl,		FALSE,	TRUE,  	IsInAppState,				APP_STATE_IN_GAME,			APP_GAME_ALL	)
#endif																																			

	DECLARE_HANDLER_TBL(gDemoModeKeyHandlerTbl,			TRUE,	FALSE, 	InDemoMode,					0, 							APP_GAME_HELLGATE)
	DECLARE_HANDLER_TBL(gConsoleToggleHandlerTbl,		FALSE,	FALSE,	IsInAppState,				APP_STATE_IN_GAME,			APP_GAME_HELLGATE) 
	DECLARE_HANDLER_TBL(gBugReportHandlerTbl,			TRUE,	FALSE,	UIComponentIsActive,		UICOMP_BUG_REPORT,			APP_GAME_ALL) 
	DECLARE_HANDLER_TBL(gMovieHandlerTbl,				TRUE,	FALSE,	IsInMovie,					APP_STATE_PLAYMOVIELIST, 	APP_GAME_HELLGATE) 
	DECLARE_HANDLER_TBL(gDialogKeyHandlerTbl,			TRUE,	FALSE,	UIComponentIsActive,		UICOMP_GENERIC_DIALOG, 		APP_GAME_ALL) 
	DECLARE_HANDLER_TBL(gGuildCreateKeyHandlerTbl,		TRUE,	FALSE,	UIComponentIsActive,		UICOMP_GUILD_CREATE_DIALOG, APP_GAME_ALL) 
	DECLARE_HANDLER_TBL(gStackSplitKeyHandlerTbl,		TRUE,	FALSE,	UIComponentIsActive,		UICOMP_STACK_SPLIT_DIALOG,	APP_GAME_ALL) 
	DECLARE_HANDLER_TBL(gInviteKeyHandlerTbl,			FALSE,	FALSE,	UIComponentIsActive,		UICOMP_PARTY_INVITE_DIALOG,	APP_GAME_ALL) 

	DECLARE_HANDLER_TBL(gStartMenuKeyHandlerTbl,		TRUE,	FALSE, 	IsInAppState,				APP_STATE_MAINMENU,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gEulaScreenKeyHandlerTbl,		TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_EULA_SCREEN,			APP_GAME_ALL	)
//	DECLARE_HANDLER_TBL(gEulaScreenKeyHandlerTbl,		TRUE,	FALSE, 	IsInAppState,				APP_STATE_TOS,				APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gGameMenuKeyHandlerTbl,			TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_GAMEMENU,			APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gChatEditKeyHandlerTbl,			TRUE,	FALSE,	ConsoleIsEditActive,		0,							APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gSocialScreenKeyHandlerTbl,		TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_SOCIAL_SCREEN,		APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gTradeKeyHandlerTbl,			FALSE,	FALSE, 	TradeIsActive,				0, 							APP_GAME_ALL	)		
	DECLARE_HANDLER_TBL(gBuddyListKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_BUDDYLIST,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gGuildMgtKeyHandlerTbl,			FALSE,	FALSE,	UIComponentIsActive,		UICOMP_GUILD_MANAGEMENT_PANEL,		APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gGuildInviteKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_GUILD_INVITE_PANEL,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gGuildPanelKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_GUILDPANEL,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gPartyPanelKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_PARTYPANEL,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gPlayerMatchKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_PLAYERMATCH_PANEL,	APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gEmailViewKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_EMAIL_VIEW_PANEL,	APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gEmailComposeKeyHandlerTbl,		FALSE,	FALSE,	UIComponentIsActive,		UICOMP_EMAIL_COMPOSE_PANEL,	APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gAuctionKeyHandlerTbl,			FALSE,	FALSE,	UIComponentIsActive,		UICOMP_AUCTION_PANEL,		APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gCharacterCreateKeyHandlerTbl,	TRUE,	FALSE, 	IsInAppState,				APP_STATE_CHARACTER_CREATE,	APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gKioskKeyHandlerTbl,			TRUE,	FALSE,	UIComponentIsActive,		UICOMP_KIOSK,				APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gVideoIncomingKeyHandlerTbl,	FALSE,	FALSE, 	IsIncomingMessage,			0,							APP_GAME_ALL	)// needs to be before chat edit toggle

	DECLARE_HANDLER_TBL(gNPCDialogKeyHandlerTbl,		TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_NPC_CONVERSATION_DIALOG,		APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gNPCVideoDialogKeyHandlerTbl,	TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_VIDEO_DIALOG,		APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gChatEditToggleKeyHandlerTbl,	FALSE,	FALSE,	IsConsoleAvailable,			0,							APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gOptionsKeyHandlerTbl,			TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_OPTIONSDIALOG,		APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gRestartKeyHandlerTbl,			TRUE,	FALSE,	IsPlayerDead,				0,							APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gAltActivebarHandlerTbl,		FALSE,	FALSE,	PlayerHasState,				STATE_USE_OTHER_HOTKEYS,	APP_GAME_HELLGATE )
	DECLARE_HANDLER_TBL(gCannonModeHandlerTbl,			TRUE,	FALSE,	PlayerHasState,				STATE_CANNON_USING,			APP_GAME_HELLGATE )
	DECLARE_HANDLER_TBL(gRTSModeHandlerTbl,				TRUE,	FALSE,	PlayerHasState,				STATE_QUEST_RTS,			APP_GAME_HELLGATE )
	DECLARE_HANDLER_TBL(gRTSModeHandlerTbl,				TRUE,	FALSE,	PlayerHasState,				STATE_QUEST_RTS_HOPE,		APP_GAME_HELLGATE )

	DECLARE_HANDLER_TBL(gOptionsMenuKeyHandlerTbl,		TRUE,   FALSE, 	UIComponentIsActive,		UICOMP_OPTIONSDIALOG,		APP_GAME_ALL	)
#if ISVERSION(DEVELOPMENT)																														
	DECLARE_HANDLER_TBL(gDetachedCameraKeyHandlerTbl,	FALSE,	TRUE,  	IsDetatchedCameraActive,	0,							APP_GAME_ALL	)
#endif																																			
	DECLARE_HANDLER_TBL(gScreenshotKeyHandlerTbl,		FALSE,	FALSE, 	InScreenshotMode,			0, 							APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gEngineDebugKeyHandlerTbl,		FALSE,	TRUE,  	DebugKeysAreActive,			0, 							APP_GAME_ALL	)
																																				
	DECLARE_HANDLER_TBL(gTugboatAutomapHandler,			FALSE,	FALSE, 	IsInAppState,				APP_STATE_IN_GAME,			APP_GAME_TUGBOAT)

	DECLARE_HANDLER_TBL(gWeaponConfigKeyHandlerTbl,		FALSE,	FALSE, 	IsInAppState,				APP_STATE_IN_GAME, 			APP_GAME_HELLGATE	)
																																				
	DECLARE_HANDLER_TBL(gActiveBarKeyHandlerTbl,		FALSE,	FALSE, 	IsInAppState,				APP_STATE_IN_GAME, 			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gHotSpellKeyHandlerTbl,			FALSE,	FALSE,	IsInAppState,				APP_STATE_IN_GAME,			APP_GAME_TUGBOAT )
																																				
	DECLARE_HANDLER_TBL(gUIActivationKeyHandlerTbl,		FALSE,	FALSE, 	UIComponentIsActive,		UICOMP_MAINSCREEN,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gInventoryKeyHandlerTbl,		TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_PLAYER_OVERVIEW_PANEL,	APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gInventoryKeyHandlerTbl,		TRUE,	FALSE, 	InventoryIsActive,			0, 							APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gSkillMapKeyHandlerTbl,			TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_SKILLMAP,			APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gSkillMapKeyHandlerTbl,			TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_PERKMAP,				APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gQuestPanelKeyHandlerTbl,		TRUE,	FALSE,	UIComponentIsActive,		UICOMP_QUEST_PANEL,			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gAchievementsPanelKeyHandlerTbl,TRUE,	FALSE,	UIComponentIsActive,		UICOMP_ACHIEVEMENTS,		APP_GAME_HELLGATE)
																																				
	DECLARE_HANDLER_TBL(gTownPortalKeyHandlerTbl,		TRUE,	FALSE, 	UIComponentIsActive,		UICOMP_TOWN_PORTAL_DIALOG,	APP_GAME_ALL	)
																																				
	DECLARE_HANDLER_TBL(gTipDialogKeyHandlerTbl,		FALSE,	FALSE, 	UIComponentIsActive,		UICOMP_TUTORIAL_DIALOG,		APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(gGameMovementKeyHandlerTbl,		FALSE,	FALSE, 	UIComponentIsActive,		UICOMP_MAINSCREEN, 			APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gJumpMovementKeyHandlerTbl,		FALSE,	FALSE, 	IsInAppState,				APP_STATE_IN_GAME, 			APP_GAME_HELLGATE	)
	DECLARE_HANDLER_TBL(gGameControlKeyHandlerTbl,		FALSE,	FALSE, 	UIComponentIsActive,		UICOMP_MAINSCREEN, 			APP_GAME_ALL	)
	DECLARE_HANDLER_TBL(gCameraControlKeyHandlerTbl,	FALSE,	FALSE, 	IsInAppState,				APP_STATE_IN_GAME, 			APP_GAME_ALL	)

	DECLARE_HANDLER_TBL(NULL,							FALSE,  FALSE,	NULL,						0, 							APP_GAME_ALL )
};




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsModKeyDown(
	void)
{
	return (GetKeyState(VK_MENU) & 0x8000) || (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_SHIFT) & 0x8000);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputHandleKey(
	GAME * game,
	UINT uMsg,
	DWORD wKey,
	DWORD lParam)
{
	UNIT * unit = NULL;

	if (wKey == VK_LBUTTON)
	{
		sInputHandleLeftButton(game, uMsg == WM_KEYDOWN);
		return TRUE;
	}
	if (wKey == VK_RBUTTON)
	{
		sInputHandleRightButton(game, uMsg == WM_KEYDOWN);
		return TRUE;
	}

	if (UIShowingLoadingScreen())
	{
		return FALSE;
	}

	if (game)
	{
		unit = GameGetControlUnit(game);
		if (unit && UnitTestFlag( unit, UNITFLAG_QUESTSTORY ) && wKey != VK_ESCAPE)
		{
			return FALSE;
		}
	}

	if (AppCommonGetAbsTime() < gInput.m_tiBlockKeyInputUntil)
	{
		return FALSE;
	}

	// ok, apparently ctrl+shift+2 sends a zero key message.  We should ignore them
	if (wKey == 0)
		return FALSE;

	BOOL bModKeyState = IsModKeyDown();
//	UINT nRepeatCount = (uMsg == WM_CHAR || uMsg == WM_KEYDOWN  || uMsg == WM_SYSKEYDOWN ? lParam & 0xff : 0);
	BOOL bPrevDown = (uMsg == WM_CHAR || uMsg == WM_KEYDOWN  || uMsg == WM_SYSKEYDOWN ? lParam & 0x40000000 : lParam & 0x4000);

	KEY_HANDLER_LIST_ENTRY * pCurKeyHndlrListEntry = gKeyHandersList;
	while (pCurKeyHndlrListEntry && pCurKeyHndlrListEntry->pKeyHandlerTbl)
	{
		if ( (!pCurKeyHndlrListEntry->bCheats || GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS) ) &&
			(pCurKeyHndlrListEntry->eGame == APP_GAME_ALL || pCurKeyHndlrListEntry->eGame == AppGameGet()) &&
			pCurKeyHndlrListEntry->fpIsActive != NULL &&
			pCurKeyHndlrListEntry->fpIsActive(pCurKeyHndlrListEntry->nIsActiveParam))
		{
			KEY_HANDLER * handler = pCurKeyHndlrListEntry->pKeyHandlerTbl;
			ASSERT_BREAK(handler)
			for (; handler->nCommand != -1; ++handler)
			{
				if (handler->eGame != APP_GAME_ALL && handler->eGame != AppGameGet())
				{
					continue;
				}
				if (handler->m_bGameRequired && !game)
				{
					continue;
				}
				if (handler->m_bUnitRequired && !unit)
				{
					continue;
				}		
				if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && 
					(!handler->fpOnKeyDown && !handler->fpOnKeyUp))	// also check for the keyup handler 'cause if it's got one we need to record the key down state
				{
					continue;
				}
				if ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && !handler->fpOnKeyUp)
				{
					continue;
				}
				if ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && !handler->bIsDown)
				{
					continue;
				}
				if (uMsg == WM_CHAR && !handler->fpOnKeyChar)
				{
					continue;
				}
				if ((uMsg == WM_CHAR || uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) &&
					handler->bNoRepeats && bPrevDown)
				{
					continue;
				}

				ASSERT_RETFALSE(handler->nCommand == -2 || (handler->nCommand >= 0 && handler->nCommand < NUM_KEY_COMMANDS));

				BOOL bModKeyDown = bModKeyState;
				// Movement keys ignore mod keys
				if (AppIsHellgate() &&
					handler->nCommand == CMD_TURN_LEFT ||		
					handler->nCommand == CMD_TURN_RIGHT ||		
					handler->nCommand == CMD_MOVE_FORWARD ||
					handler->nCommand == CMD_MOVE_BACK ||
					handler->nCommand == CMD_MOVE_LEFT ||
					handler->nCommand == CMD_MOVE_RIGHT)
				{
					bModKeyDown = FALSE;
				}

				for (unsigned int ii = 0; ii < NUM_KEYS_PER_COMMAND; ii++)
				{
					if (handler->nCommand == -2 || 
						(g_pKeyAssignments[handler->nCommand].KeyAssign[ii].nKey == (int)wKey && 
						 (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP ||
						  (g_pKeyAssignments[handler->nCommand].KeyAssign[ii].nModifierKey == 0 && !bModKeyDown) || 
						  (GetKeyState(g_pKeyAssignments[handler->nCommand].KeyAssign[ii].nModifierKey) & 0x8000))))
					{
						DWORD data = (DWORD)handler->data;

						switch (uMsg)
						{
						case WM_HOTKEY:
						case WM_SYSKEYDOWN:
						case WM_KEYDOWN:
							{
								handler->bIsDown = TRUE;
								if (handler->fpOnKeyDown)
								{
									if (handler->fpOnKeyDown(game, unit, handler->nCommand, wKey, lParam, data))
									{
										#if ISVERSION(DEVELOPMENT)
										if (gApp.m_bKeyHandlerLogging)
										{
											ConsoleString(CONSOLE_SYSTEM_COLOR, L"key '%c' down handled by %s - %s:%s", (char)wKey, pCurKeyHndlrListEntry->szDebugName, handler->szCommand, handler->szOnKeyDown );
										}
										#endif
										return TRUE;
									}
									#if ISVERSION(DEVELOPMENT)
									else if (gApp.m_bKeyHandlerLogging)
									{
										ConsoleString(CONSOLE_SYSTEM_COLOR, L"key '%c' down NOT handled by %s - %s:%s", (char)wKey, pCurKeyHndlrListEntry->szDebugName, handler->szCommand, handler->szOnKeyDown );
									}
									#endif									
								}
							}
							break;
						case WM_SYSKEYUP:
						case WM_KEYUP:
							handler->bIsDown = FALSE;
							ASSERT(handler->fpOnKeyUp);
							if (handler->fpOnKeyUp(game, unit, handler->nCommand, wKey, lParam, data))
							{
								#if ISVERSION(DEVELOPMENT)
								if (gApp.m_bKeyHandlerLogging)
								{
									ConsoleString(CONSOLE_SYSTEM_COLOR, L"key '%c'  up  handled by %s - %s:%s", (char)wKey, pCurKeyHndlrListEntry->szDebugName, handler->szCommand, handler->szOnKeyUp );
								}
								#endif
								return TRUE;
							}
							#if ISVERSION(DEVELOPMENT)
							else if (gApp.m_bKeyHandlerLogging)
							{
								ConsoleString(CONSOLE_SYSTEM_COLOR, L"key '%c'  up  NOT handled by %s - %s:%s", (char)wKey, pCurKeyHndlrListEntry->szDebugName, handler->szCommand, handler->szOnKeyDown );
							}
							#endif									
							break;
						case WM_CHAR:
							ASSERT(handler->fpOnKeyChar);
							if (handler->fpOnKeyChar(game, unit, handler->nCommand, wKey, lParam, data))
							{
								#if ISVERSION(DEVELOPMENT)
								if (gApp.m_bKeyHandlerLogging)
								{
									ConsoleString(CONSOLE_SYSTEM_COLOR, L"key '%c' char handled by %s - %s:%s", (char)wKey, pCurKeyHndlrListEntry->szDebugName, handler->szCommand, handler->szOnKeyChar );
								}
								#endif
								return TRUE;
							}
							#if ISVERSION(DEVELOPMENT)
							else if (gApp.m_bKeyHandlerLogging)
							{
								ConsoleString(CONSOLE_SYSTEM_COLOR, L"key '%c' char NOT handled by %s - %s:%s", (char)wKey, pCurKeyHndlrListEntry->szDebugName, handler->szCommand, handler->szOnKeyDown );
							}
							#endif									
							break;
						}
					}
				}
			}

			if (pCurKeyHndlrListEntry->bBlocking)
			{
				#if ISVERSION(DEVELOPMENT)
				if (gApp.m_bKeyHandlerLogging)
				{
					ConsoleString(CONSOLE_SYSTEM_COLOR, L"Handler table %s blocks further key handling", pCurKeyHndlrListEntry->szDebugName );
				}
				#endif
				break;
			}
		}

		pCurKeyHndlrListEntry++;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputExecuteKeyCommand(
	int nCommand,
	DWORD lParam /*= 0*/)
{
	UNIT* unit = NULL;
	GAME* game = AppGetCltGame();
	if (game)
	{
		unit = GameGetControlUnit(game);
		if ( unit && UnitTestFlag( unit, UNITFLAG_QUESTSTORY ) )
			return FALSE;
	}
	KEY_HANDLER_LIST_ENTRY * pCurKeyHndlrListEntry = gKeyHandersList;
	while (pCurKeyHndlrListEntry && pCurKeyHndlrListEntry->pKeyHandlerTbl)
	{
		if (pCurKeyHndlrListEntry->fpIsActive != NULL &&
			pCurKeyHndlrListEntry->fpIsActive(pCurKeyHndlrListEntry->nIsActiveParam))
		{
			KEY_HANDLER * handler = pCurKeyHndlrListEntry->pKeyHandlerTbl;
			while (handler->nCommand != -1)
			{
				if (handler->nCommand == nCommand && 
					(!handler->m_bGameRequired || game) && (!handler->m_bUnitRequired || unit))
				{
					ASSERT_RETFALSE(handler->nCommand == -2 || (handler->nCommand >= 0 && handler->nCommand < NUM_KEY_COMMANDS));
					DWORD data = (DWORD)handler->data;
					if (handler->fpOnKeyDown)
					{
						handler->fpOnKeyDown(game, unit, handler->nCommand, 0, lParam, data);
					}
					if (handler->fpOnKeyUp)
					{
						handler->fpOnKeyUp(game, unit, handler->nCommand, 0, lParam, data);
					}
					return TRUE;
				}
				handler++;
			}

			if (pCurKeyHndlrListEntry->bBlocking)
				break;
		}
		pCurKeyHndlrListEntry++;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
BOOL sHandleCheatKeyInputDownUp(
	GAME * game,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if (!game || !CHEAT_CODE)
	{
		gInput.m_nCheatCodeMatch = 0;
		return FALSE;
	}
	if (gInput.m_nCheatCodeMatch <= gInput.m_nCheatCodeLen)
	{
		if ((int)wParam == (int)CHEAT_CODE[gInput.m_nCheatCodeMatch])
		{
			if (uMsg == WM_KEYUP)
			{
				gInput.m_nCheatCodeMatch++;
				if (gInput.m_nCheatCodeMatch >= gInput.m_nCheatCodeLen)
				{
					GameToggleDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS);
					BOOL bAllowCheats = GameGetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS);

					GAME * srvGame = AppGetSrvGame();
					if (srvGame)
					{
						GameSetDebugFlag(srvGame, DEBUGFLAG_ALLOW_CHEATS, bAllowCheats);
					}
					ConsoleString(CONSOLE_SYSTEM_COLOR, "Cheats %s", bAllowCheats ? "Enabled" : "Disabled");
				}
			}
			return TRUE;
		}
	}
	gInput.m_nCheatCodeMatch = 0;
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LRESULT c_InputHandleKeys(
	GAME * game,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if ( ! e_OptionState_IsInitialized() )
		return 0;

	switch (uMsg)
	{
	case WM_HOTKEY:
		if ( wParam != HGVK_SCREENSHOT_UI && wParam != HGVK_SCREENSHOT_NO_UI )
			break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
#if ISVERSION(CHEATS_ENABLED)
		if (sHandleCheatKeyInputDownUp(game, uMsg, wParam, lParam))
		{
			return 0;
		}
#endif
		InputHandleKey(game, uMsg, (DWORD)wParam, (DWORD)lParam);
		return 0;

	case WM_CHAR:
		InputHandleKey(game, uMsg, (DWORD)wParam, (DWORD)lParam);
		return 0;
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LRESULT c_InputHandleMouseButton(
	GAME * game,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if ( ! e_OptionState_IsInitialized() )
		return 0;

	if (c_UsingDirectInput())
	{
		return 0;
	}

	if (AppIsHellgate())
		return sHandleMouseButtonHellgate(game, uMsg, wParam, lParam);

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LRESULT c_InputHandleMouseMove(
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if ( ! e_OptionState_IsInitialized() )
		return 0;

	if (c_UsingDirectInput())
	{
		return 0;
	}

	if ( AppIsMenuLoop() )
	{
		return 0;
	}

	if ( AppIsMoveWindow() )
	{
		return 0;
	}

	if (uMsg == WM_INPUT)
	{
		GAME *game = AppGetCltGame();
		UINT dwSize = sizeof(RAWINPUT);
		static RAWINPUT rawdata;

		UINT nReturn = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, 
						&rawdata, &dwSize, sizeof(RAWINPUTHEADER));

		if (nReturn != (UINT)-1 &&
			rawdata.header.dwType == RIM_TYPEMOUSE) 
		{
			float fDeltaX = (float)rawdata.data.mouse.lLastX;
			float fDeltaY = (float)rawdata.data.mouse.lLastY;
			//if (nDeltaX || nDeltaY)
			{
				if (!AppIsHellgate() || UICursorGetActive())
				{
					if (AppIsTugboat() )
						
					{


						if( c_CameraGetViewType() == VIEW_VANITY )
						{
							BOOL OverPanel = UI_TB_MouseOverPanel();
							if( !OverPanel && ( InputGetMouseButtonDown( MOUSE_BUTTON_LEFT ) || InputIsCommandKeyDown( CMD_CAMERAORBIT ) ) )
							{
								if (fDeltaX != 0)
								{
									fDeltaX = ((float)fDeltaX * gInput.m_fMouseLookSensitivity * 2);
									PlayerMouseRotate(game, fDeltaX);
								}
								if (fDeltaY != 0 )
								{
									fDeltaY = ((float)fDeltaY * gInput.m_fMouseLookSensitivity * 2);
									if (InputGetMouseInverted())
										PlayerMousePitch(game, fDeltaY);
									else
										PlayerMousePitch(game, -fDeltaY);
								}
								if( gPreOrbitCursorPosX == -1 &&
									gPreOrbitCursorPosY == -1 )
								{
									POINT pt;
									::GetCursorPos(&pt);
									gPreOrbitCursorPosX = pt.x;
									gPreOrbitCursorPosY = pt.y;
								}

								::SetCursorPos(gInput.m_nScreenCenterX, gInput.m_nScreenCenterY);

							}
							else
							{
								::ShowCursor( TRUE );
								UISetCursorState( UIGetCursorState(), 0 );
							}
						}
						else if( InputGetAdvancedCamera() )
						{
							if ( InputIsCommandKeyDown( CMD_CAMERAORBIT ) )
							{
								if (fDeltaX != 0)
								{
									fDeltaX = ((float)fDeltaX * gInput.m_fMouseLookSensitivity * 2);
									PlayerMouseRotate(game, fDeltaX);
								}
								if (fDeltaY != 0 && !InputGetLockedPitch() )
								{
									fDeltaY = ((float)fDeltaY * gInput.m_fMouseLookSensitivity * 2);
									if (InputGetMouseInverted())
										PlayerMousePitch(game, fDeltaY);
									else
										PlayerMousePitch(game, -fDeltaY);
								}
								if( gPreOrbitCursorPosX == -1 &&
									gPreOrbitCursorPosY == -1 )
								{
									POINT pt;
									::GetCursorPos(&pt);
									gPreOrbitCursorPosX = pt.x;
									gPreOrbitCursorPosY = pt.y;
								}
								::SetCursorPos(gInput.m_nScreenCenterX, gInput.m_nScreenCenterY);

							}
							else
							{
								::ShowCursor( TRUE );
								UISetCursorState( UIGetCursorState(), 0 );
							}
						}
						else
						{
							::ShowCursor( TRUE );
							UISetCursorState( UIGetCursorState(), 0 );
						}
						
					}

					POINT pt;
					::GetCursorPos(&pt);

					HWND hwnd = AppCommonGetHWnd();
					::ScreenToClient(hwnd, &pt);
					if (!sInputInternalSetMousePosition((float)pt.x, (float)pt.y))
					{
						// cursor is outside the window
						if (InputGetMouseAcquired())
						{
							c_InputMouseAcquire(game, FALSE);
						}
						return 0;
					}
				}
				else
				{
					if ( !AppCommonIsPaused() )
					{
						if (fDeltaX != 0)
						{
							fDeltaX = ((float)fDeltaX * gInput.m_fMouseLookSensitivity);
							PlayerMouseRotate(game, fDeltaX);
						}
						if (fDeltaY != 0)
						{
							fDeltaY = ((float)fDeltaY * gInput.m_fMouseLookSensitivity);
							if (InputGetMouseInverted())
								PlayerMousePitch(game, fDeltaY);
							else
								PlayerMousePitch(game, -fDeltaY);
						}
		
						::SetCursorPos(gInput.m_nScreenCenterX, gInput.m_nScreenCenterY);
					}
				}
			}

			if (!InputGetMouseAcquired())
			{
				c_InputMouseAcquire(game, TRUE);
			}

			gInput.m_ulRawButtons = rawdata.data.mouse.ulRawButtons;

#if (0)
			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)		sInputHandleLeftButton(game, TRUE);
			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)			sInputHandleLeftButton(game, FALSE);

			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)		sInputHandleRightButton(game, TRUE);
			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)		sInputHandleRightButton(game, FALSE);

			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)		InputHandleKey(game, WM_KEYDOWN, VK_MBUTTON, 0);
			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)		InputHandleKey(game, WM_KEYUP, VK_MBUTTON, 0);

			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)			InputHandleKey(game, WM_KEYDOWN, VK_XBUTTON1, 0);
			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)			InputHandleKey(game, WM_KEYUP, VK_XBUTTON1, 0);

			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)			InputHandleKey(game, WM_KEYDOWN, VK_XBUTTON2, 0);
			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)			InputHandleKey(game, WM_KEYUP, VK_XBUTTON2, 0);
#else
			for (int iBtn = 0; iBtn < MOUSE_BUTTON_COUNT; iBtn++)
			{
				int nVK = GetMouseVKButton(iBtn);
				BOOL bBtnDown = ((GetAsyncKeyState(nVK) & 0x8000) != 0);
				if (bBtnDown != InputGetMouseButtonDown(iBtn))
				{
					SETBIT(&gInput.m_dwMouseButtonState, iBtn, bBtnDown);

					InputHandleKey(game, bBtnDown ? WM_KEYDOWN : WM_KEYUP, nVK, 0);
				}
			}
#endif

		// except it doesn't seem to work for the middle mouse button
			//if (TESTBIT(rawdata.data.mouse.ulRawButtons, MOUSE_BUTTON_MIDDLE) != TESTBIT(gInput.m_dwMouseButtonState, MOUSE_BUTTON_MIDDLE))
			//{
			//	TOGGLEBIT(gInput.m_dwMouseButtonState, MOUSE_BUTTON_MIDDLE);
			//	InputHandleKey(game, TESTBIT(rawdata.data.mouse.ulRawButtons, MOUSE_BUTTON_MIDDLE) ? WM_KEYDOWN : WM_KEYUP, VK_MBUTTON, 0);
			//}

			if (rawdata.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)					sInputHandleMouseWheelDelta((short int)rawdata.data.mouse.usButtonData);

			if( !AppIsHellgate() )
			{
				c_InputTugboatMouseUpdate(game);
			}

			return TRUE;
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LRESULT c_InputHandleNCMouseMove(
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if (c_UsingDirectInput())
	{
		return 0;
	}

	if (uMsg == WM_NCMOUSEMOVE)
	{
		GAME *game = AppGetCltGame();
		if (InputGetMouseAcquired())
		{
			c_InputMouseAcquire(game, FALSE);
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sInitKeyCmd(
	void)
{	// technically slow, but only done on init
	for (int i=0; i < NUM_KEY_COMMANDS; i++)
	{
		KEY_COMMAND& key = g_pKeyAssignments[i];

		key.eGame = APP_GAME_UNKNOWN;		// if it's not found in any command lists it won't be shown for remapping
		key.nStringIndex = StringTableGetStringIndexByKey(key.szNameStringKey ? key.szNameStringKey : "");

		KEY_HANDLER_LIST_ENTRY * pCurKeyHndlrListEntry = gKeyHandersList;
		while (pCurKeyHndlrListEntry && pCurKeyHndlrListEntry->pKeyHandlerTbl)
		{
			KEY_HANDLER * handler = pCurKeyHndlrListEntry->pKeyHandlerTbl;
			ASSERT_BREAK(handler)
			for (; handler->nCommand != -1; ++handler)
			{
				if (handler->nCommand == key.nCommand)
				{
					APP_GAME eGame = (pCurKeyHndlrListEntry->eGame != APP_GAME_ALL ? pCurKeyHndlrListEntry->eGame : handler->eGame);
					if (key.eGame == APP_GAME_UNKNOWN)
					{
						key.eGame = eGame;
					}
					else if (key.eGame == APP_GAME_HELLGATE &&
						eGame == APP_GAME_TUGBOAT)
					{
						key.eGame = APP_GAME_ALL;
					}
					else if (key.eGame == APP_GAME_TUGBOAT &&
						eGame == APP_GAME_HELLGATE)
					{
						key.eGame = APP_GAME_ALL;
					}
				}
			}
			pCurKeyHndlrListEntry++;
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InputFlushKeyboardBuffer(void)
{
	int n;
	WCHAR pwszBuff[20];
	BYTE lpKeyState[256];
	const UINT vKey = VK_DECIMAL;
	UINT scanCode = MapVirtualKey(vKey, 0);

	memclear(lpKeyState, sizeof(lpKeyState));

	do
	{
		n = ToUnicode(      
			(UINT)vKey, //UINT wVirtKey,
			(UINT)scanCode, //UINT wScanCode,
			(const BYTE *)lpKeyState,
			pwszBuff,
			sizeof(pwszBuff)/sizeof(WCHAR),
			0);
	}
	while (n != 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InputGetKeyNameW(int nVirtualKey, int nModifierKey, WCHAR *szRetString, int nStrLen)
{
	const WCHAR *szKeyConfigName = KeyConfigGetNameFromKeyCode((WORD)nVirtualKey);
	const WCHAR *szKeyConfigModName;

	if (nModifierKey && nModifierKey != nVirtualKey)
	{
		szKeyConfigModName = KeyConfigGetNameFromKeyCode(nModifierKey);
	}
	else
	{
		szKeyConfigModName = NULL;
	}

	if (szKeyConfigName)
	{
		if (szKeyConfigModName)
		{
			PStrPrintf(szRetString, nStrLen, L"%s+%s", szKeyConfigModName, szKeyConfigName );
		}
		else
		{
			PStrCopy(szRetString, szKeyConfigName, nStrLen);
		}

		return TRUE;
	}

	LONG lParam = MapVirtualKey(nVirtualKey, 0);
	switch(nVirtualKey) 
	{
		case VK_MBUTTON:
		case VK_XBUTTON1:
		case VK_XBUTTON2:
		case HGVK_MOUSE_WHEEL_DOWN:
		case HGVK_MOUSE_WHEEL_UP:
		{
			const WCHAR *szKey = KeyConfigGetNameFromKeyCode((WORD)nVirtualKey);
			if (!szKey)
				return FALSE;

			PStrCopy(szRetString, szKey, nStrLen);
			return TRUE;
		}
		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_NEXT:  // Page down
		case VK_PRIOR: // Page up
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		{
			lParam |= 0x100; // Add extended bit
			break;
		}
	};

	WCHAR szKeyName[256] = {0};
	BYTE lpKeyState[256];
	memclear(lpKeyState, sizeof(lpKeyState));

	InputFlushKeyboardBuffer();

	int n = ToUnicode(      
		(UINT)nVirtualKey, //UINT wVirtKey,
		(UINT)lParam, //UINT wScanCode,
		(const BYTE *)lpKeyState,
		szKeyName,
		255,
		0);

	if (n < 0)
	{
		// mapped to a dead key...
		//
		if (ToUnicode(
			(UINT)nVirtualKey, //UINT wVirtKey,
			(UINT)lParam, //UINT wScanCode,
			(const BYTE *)lpKeyState,
			szKeyName,
			255,
			0) > 0)
		{
			n = 1;
		}
	}

	if (n > 0)
	{
		szKeyName[n] = 0;
		PStrUpperL(szKeyName, n+1);
	}
	else
	{
		GetKeyNameTextW(lParam << 16, szKeyName, 256);
	}

	if (szKeyName[0])
	{
		if (szKeyConfigModName)
		{
			PStrPrintf(szRetString, nStrLen, L"%s+%s", szKeyConfigModName, szKeyName );
		}
		else
		{
			PStrCopy(szRetString, szKeyName, nStrLen);
		}

		return TRUE;
	}

	return FALSE;
}

void InputKeyIgnore(
	DWORD dwDuration)
{
	gInput.m_tiBlockKeyInputUntil = AppCommonGetAbsTime() + dwDuration;
}

void InputMouseBtnIgnore(
	DWORD dwDuration)
{
	gInput.m_tiBlockMouseBtnUntil = AppCommonGetAbsTime() + dwDuration;
}

#endif //!SERVER_VERSION
