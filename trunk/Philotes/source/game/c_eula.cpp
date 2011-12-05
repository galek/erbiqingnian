#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)
#include "prime.h"
#include "c_eula.h"
#include "pakfile.h"
#include "uix.h"
#include "uix_priv.h"
#include "filepaths.h"
#include "uix_components.h"
#include "ServerSuite\BillingProxy\c_BillingClient.h"
#include "pakfile.h"
#include "language.h"
#include "sku.h"

static EULA_STATE gseAppEulaState = EULA_STATE_NONE;

static void c_sEulaStartCharSelect()
{
//	UIComponentActivate(UICOMP_CHARACTER_SELECT, TRUE);
}

void c_EulaStateReset()
{
	InterlockedExchange((LONG*)&gApp.m_bEulaHashReady, 0);
	InterlockedExchange((LONG*)&gApp.m_iNewEulaHash, 0);
	
	gseAppEulaState = EULA_STATE_NONE;
}

static void* c_sEulaLoadTextCalculateCRC(
	LPCTSTR filename,
	MEMORYPOOL* pPool,
	DWORD& dwCRC)
{
	DECLARE_LOAD_SPEC(spec, filename);

	void* buffer = PakFileLoadNow(spec);
	if (buffer) {
		dwCRC = CRC(dwCRC, (const BYTE*)buffer, spec.bytesread);
		((LPWSTR)buffer)[spec.bytesread/2-1] = L'\0';
	}
	return buffer;
}

static void* c_sEulaLoadTextCalculateCRC_EULA(MEMORYPOOL* pPool, DWORD& dwCRC)
{
	TCHAR filename[DEFAULT_FILE_WITH_PATH_SIZE];
	if (AppIsHellgate()) {
		LPCSTR pszLanguageName = LanguageGetCurrentName();
		ASSERT_RETNULL(pszLanguageName != NULL);

		const SKU_DATA* pSkuData = SKUGetData(SKUGetCurrent());
		ASSERT_RETNULL(pSkuData);

		PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%sdocs\\%s\\HG_EULA_%s.txt", FILE_PATH_DATA, pSkuData->szName, pszLanguageName);
	} else if (AppIsTugboat()) {
		PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, "eula.txt");
	}
	return c_sEulaLoadTextCalculateCRC(filename, pPool, dwCRC);
}

static void* c_sEulaLoadTextCalculateCRC_TOS(MEMORYPOOL* pPool, DWORD& dwCRC)
{
	TCHAR filename[DEFAULT_FILE_WITH_PATH_SIZE];
	if (AppIsHellgate()) {
		LPCSTR pszLanguageName = LanguageGetCurrentName();
		ASSERT_RETNULL(pszLanguageName != NULL);

		const SKU_DATA* pSkuData = SKUGetData(SKUGetCurrent());
		ASSERT_RETNULL(pSkuData);

		PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%sdocs\\%s\\HG_TOS_%s.txt", FILE_PATH_DATA, pSkuData->szName, pszLanguageName);
	} else if (AppIsTugboat()) {
		PStrPrintf(filename, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", FILE_PATH_DATA, "tos.txt");
	}
	return c_sEulaLoadTextCalculateCRC(filename, pPool, dwCRC);
}


static void c_sEulaInitCalculateCRC()
{
	DWORD dwCRC = 0;
	void* pBuffer;

	pBuffer = c_sEulaLoadTextCalculateCRC_EULA(NULL, dwCRC);
	if (pBuffer) {
		FREE(NULL, pBuffer);
	}

	pBuffer = c_sEulaLoadTextCalculateCRC_TOS(NULL, dwCRC);
	if (pBuffer) {
		FREE(NULL, pBuffer);
	}

	AppSetNewEulaHash(dwCRC);
	gseAppEulaState = EULA_STATE_WAITING_FOR_HASH;
}

static void c_sEulaUILoadEULA()
{
	DWORD dwCRC = 0;
	UI_TEXTBOX* pEulaText = UICastToTextBox(UIComponentGetByEnum(UICOMP_EULA_TEXT));
	if (pEulaText != NULL) {
		void* pBuffer = c_sEulaLoadTextCalculateCRC_EULA(NULL, dwCRC);
		if (pBuffer) {
			UITextBoxClear(pEulaText);
			UITextBoxAddLine(pEulaText, (LPCWSTR)pBuffer);
			FREE(NULL, pBuffer);
		} else {
			UITextBoxClear(pEulaText);
			UITextBoxAddLine(pEulaText, L"End User License Agreement - Not Found");
			if (AppIsHellgate()) {
				UITextBoxAddLine(pEulaText, L"http://alpha.hellgatelondon.com/eula");
			} else if (AppIsTugboat()) {
				UITextBoxAddLine(pEulaText, L"http://www.mythos.com/EULA.jsp");
			}
		}
		pEulaText->m_ScrollPos.m_fX = 0;
		pEulaText->m_ScrollPos.m_fY = 0;
		UIComponentHandleUIMessage(pEulaText, UIMSG_PAINT, 0, 0);
	}
}

static void c_sEulaUILoadTOS()
{
	DWORD dwCRC = 0;
	UI_TEXTBOX* pEulaText = UICastToTextBox(UIComponentGetByEnum(UICOMP_EULA_TEXT));
	if (pEulaText != NULL) {
		void* pBuffer = c_sEulaLoadTextCalculateCRC_TOS(NULL, dwCRC);
		if (pBuffer) {
			UITextBoxClear(pEulaText);
			UITextBoxAddLine(pEulaText, (LPCWSTR)pBuffer);
			FREE(NULL, pBuffer);
		} else {
			UITextBoxClear(pEulaText);
			UITextBoxAddLine(pEulaText, L"Terms of Service - Not Found");
			if (AppIsHellgate()) {
				UITextBoxAddLine(pEulaText, L"http://alpha.hellgatelondon.com/terms");
			} else if (AppIsTugboat()) {
				UITextBoxAddLine(pEulaText, L"http://www.mythos.com/terms_of_service.jsp");
			}
		}
		pEulaText->m_ScrollPos.m_fX = 0;
		pEulaText->m_ScrollPos.m_fY = 0;
		UIComponentHandleUIMessage(pEulaText, UIMSG_PAINT, 0, 0);
	}
}


static void c_sEulaWaitForHash()
{
	DWORD iOldEulaHash = 0, iNewEulaHash = 0;
	if (AppGetOldEulaHash(iOldEulaHash) == FALSE) {
		// if we didn't get the old eula has from the server yet, do nothing
		return;
	}

	AppGetNewEulaHash(iNewEulaHash);
	if (iOldEulaHash == iNewEulaHash) {
		gseAppEulaState = EULA_STATE_DONE;
		c_sEulaStartCharSelect();
	} else {
		gseAppEulaState = EULA_STATE_EULA;
		c_sEulaUILoadEULA();
		UIComponentActivate(UICOMP_EULA_SCREEN, TRUE);
	}
}

BOOL c_EulaStateUpdate()
{
	// Only show EULA & TOS in closed client mode
	if (AppGetType() != APP_TYPE_CLOSED_CLIENT) {
		if (gseAppEulaState == EULA_STATE_NONE) {
			gseAppEulaState = EULA_STATE_DONE;
			c_sEulaStartCharSelect();
		}
		return TRUE;
	}

	BOOL bDone = FALSE;
	switch (gseAppEulaState) {
		case EULA_STATE_DONE:
			bDone = TRUE;
			break;
		case EULA_STATE_NONE:
			c_sEulaInitCalculateCRC();
			break;
		case EULA_STATE_WAITING_FOR_HASH:
			c_sEulaWaitForHash();
			if (gseAppEulaState == EULA_STATE_DONE) {
				bDone = TRUE;
			}
			break;
		case EULA_STATE_EULA:
		case EULA_STATE_TOS:
		default:
			break;
	}
	return bDone;
}

EULA_STATE c_EulaGetState()
{
	return gseAppEulaState;
}

//----------------------------------------------------------------------------
// Defined in uix_components.cpp
//----------------------------------------------------------------------------
void UISetCurrentMenu(UI_COMPONENT * pMenu);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEulaCancelOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg != UIMSG_KEYUP && !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYUP && wParam != VK_ESCAPE)
		return UIMSG_RET_NOT_HANDLED;

	UIComponentActivate(UICOMP_EULA_SCREEN, FALSE);

	AppSetType(gApp.eStartupAppType);

//	if (!AppIsTugboat()) {
//		UISetCurrentMenu(UIGetMainMenu());
//	}
	AppSwitchState(APP_STATE_RESTART);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEulaOkOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg != UIMSG_KEYUP && !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYUP && wParam != VK_RETURN)
		return UIMSG_RET_NOT_HANDLED;

	ASSERT_RETVAL(gseAppEulaState == EULA_STATE_EULA || gseAppEulaState == EULA_STATE_TOS, UIMSG_RET_NOT_HANDLED);
	if (gseAppEulaState == EULA_STATE_EULA) {
		c_sEulaUILoadTOS();
		gseAppEulaState = EULA_STATE_TOS;
	} else {
		gseAppEulaState = EULA_STATE_DONE;
		UIComponentActivate(UICOMP_EULA_SCREEN, FALSE);
		DWORD dwNewEulaHash = 0;
		AppGetNewEulaHash(dwNewEulaHash);
		c_BillingSendNewEulaHash(dwNewEulaHash);
		c_sEulaStartCharSelect();
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}




#endif // SERVER_VERSION
