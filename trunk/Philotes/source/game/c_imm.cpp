#include "stdafx.h"

#if !ISVERSION(SERVER_VERSION)
#include <imm.h>
#include "c_imm.h"
#include "appcommon.h"
#include "uix_priv.h"

#define	MAX_COMPSTRING_SIZE	256

#define CHT_IMEFILENAME1    "TINTLGNT.IME" // New Phonetic
#define CHT_IMEFILENAME2    "CINTLGNT.IME" // New Chang Jie
#define CHT_IMEFILENAME3    "phon.ime" // Phonetic 5.1
#define CHS_IMEFILENAME1    "PINTLGNT.IME" // MSPY1.5/2/3
#define CHS_IMEFILENAME2    "MSSCIPYA.IME" // MSPY3 for OfficeXP

#define LANG_CHT            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL)
#define LANG_CHS            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
#define _CHT_HKL            ( (HKL)(INT_PTR)0xE0080404 ) // New Phonetic
#define _CHT_HKL2           ( (HKL)(INT_PTR)0xE0090404 ) // New Chang Jie
#define _CHS_HKL            ( (HKL)(INT_PTR)0xE00E0804 ) // MSPY
#define MAKEIMEVERSION( major, minor )      ( (DWORD)( ( (BYTE)( major ) << 24 ) | ( (BYTE)( minor ) << 16 ) ) )

/*static INPUTCONTEXT* (WINAPI * _ImmLockIMC)( HIMC );
static BOOL (WINAPI * _ImmUnlockIMC)( HIMC );
static LPVOID (WINAPI * _ImmLockIMCC)( HIMCC );
static BOOL (WINAPI * _ImmUnlockIMCC)( HIMCC );*/
static HIMC (WINAPI * _ImmCreateContext)(void);
static BOOL (WINAPI * _ImmDestroyContext)(HIMC);
static BOOL (WINAPI * _ImmDisableTextFrameService)( DWORD );
static LONG (WINAPI * _ImmGetCompositionStringW)( HIMC, DWORD, LPVOID, DWORD );
static DWORD (WINAPI * _ImmGetCandidateListW)( HIMC, DWORD, LPCANDIDATELIST, DWORD );
static HIMC (WINAPI * _ImmGetContext)( HWND );
static BOOL (WINAPI * _ImmReleaseContext)( HWND, HIMC );
static HIMC (WINAPI * _ImmAssociateContext)( HWND, HIMC );
static BOOL (WINAPI * _ImmGetOpenStatus)( HIMC );
static BOOL (WINAPI * _ImmSetOpenStatus)( HIMC, BOOL );
static BOOL (WINAPI * _ImmGetConversionStatus)( HIMC, LPDWORD, LPDWORD );
static HWND (WINAPI * _ImmGetDefaultIMEWnd)( HWND );
static UINT (WINAPI * _ImmGetIMEFileNameA)( HKL, LPSTR, UINT );
static UINT (WINAPI * _ImmGetVirtualKey)( HWND );
static BOOL (WINAPI * _ImmNotifyIME)( HIMC, DWORD, DWORD, DWORD );
static BOOL (WINAPI * _ImmSetConversionStatus)( HIMC, DWORD, DWORD );
static BOOL (WINAPI * _ImmSimulateHotKey)( HWND, DWORD );
static BOOL (WINAPI * _ImmIsIME)( HKL );

// Function pointers: Traditional Chinese IME
static UINT (WINAPI * _GetReadingString)( HIMC, UINT, LPWSTR, PINT, BOOL*, PUINT );
static BOOL (WINAPI * _ShowReadingWindow)( HIMC, BOOL );

// Function pointers: Verion library imports
static BOOL (APIENTRY * _VerQueryValueA)( const LPVOID, LPSTR, LPVOID *, PUINT );
static BOOL (APIENTRY * _GetFileVersionInfoA)( LPSTR, DWORD, DWORD, LPVOID );
static DWORD (APIENTRY * _GetFileVersionInfoSizeA)( LPSTR, LPDWORD );

#define IMM32_DLLNAME L"imm32.dll"
#define VER_DLLNAME L"version.dll"

#define GETPROCADDRESS( Module, APIName, Temp ) \
	Temp = GetProcAddress( Module, #APIName ); \
	if( Temp ) \
	*(FARPROC*)&_##APIName = Temp


#define MAX_CANDIDATE_SIZE 16
#define MAX_CANDIDATE_COUNT 32

#pragma warning (push)
#pragma warning (disable:4428)
struct {
	IME_TYPE	imeType;
	char*		imeFilename;
	WCHAR*		imeLabel;
} listIme[] = {
	{	IME_CHT_PHONETIC,				"phon.ime",					L"\u6ce8\u97f3"					},
	{	IME_CHT_NEW_PHONETIC,			"TINTLGNT.IME",				L"\u65b0\u6ce8\u97f3"			},
	{	IME_CHT_CHANGJIE,				"chajei.ime",				L"\u5009\u9821"					},
	{	IME_CHT_NEW_CHANGJIE,			"CINTLGNT.IME",				L"\u65b0\u5009\u9821"			},
	{	IME_CHT_UNICODE,				"unicdime.ime",				L"Unicode"						},
	{	IME_CHT_DAYI,					"dayi.ime",					L"\u5927\u6613"					},
	{	IME_CHT_QUICK,					"quick.ime",				L"\u7c21\u6613"					},
	{	IME_CHT_BIG5,					"winime.ime",				L"\u5167\u78bc"					},
	{	IME_CHT_ARRAY,					"winar30.ime",				L"\u884c\u5217"					},
	
	{	IME_CHT_BOSHIAMY,				"LIUNT.IME",				L"\u7121\u8766\u7c73"			},
	{	IME_CHT_CHEWING,				"CHEWING.IME",				L"\u65b0\u9177\u97f3"			},
	{	IME_CHT_GOING80,				"GOING8.IME",				L"\u81ea\u7136"					},
	{	IME_CHT_GOING75,				"going7.ime",				L"\u81ea\u7136"					},

	{	IME_CHS_MSPY,					"pintlgnt.ime",				L"\u62fc\u97f3"					},
	{	IME_CHS_NEIMA,					"wingb.ime",				L"\u5185\u7801"					},
	{	IME_CHS_QUANPIN,				"winpy.ime",				L"\u5168\u62fc"					},
	{	IME_CHS_SHUANGPIN,				"winsp.ime",				L"\u53cc\u62fc"					},
	{	IME_CHS_ZHENGMA,				"winzm.ime",				L"\u90d1\u7801"					}
};
#pragma warning (pop)

class CAutoIMContext
{
public:
	CAutoIMContext(HWND hWnd) {
		m_hWnd = hWnd;
		m_hImc = _ImmGetContext(hWnd);
	}
	~CAutoIMContext() {
		if (m_hImc != NULL) {
			_ImmReleaseContext(m_hWnd, m_hImc);
		}
	}

	HIMC getContextHandle() {
		return m_hImc;
	}

private:
	HWND m_hWnd;
	HIMC m_hImc;
};

static WCHAR s_aszIndicator[5] = { // String to draw to indicate current input locale
	L'A',
	0x7B80,
	0x7E41,
	0xAC00,
	0x3042,
};

static WCHAR s_aszLanguage[5][4] = { // String to draw to indicate current input locale
	L"Eng",
	L"ChS",
	L"ChT",
	L"Krn",
	L"Jpn",
};

static HIMC s_hIMC = NULL;
static BOOL s_bEnableImeSystem = FALSE;
static HKL s_hklCurrent;
static CInputLocale* s_listLocales = NULL;
static UINT32 s_countLocales = 0;
static MEMORYPOOL* s_pPool = NULL;
static BOOL s_bVerticalCand = FALSE;
static LPWSTR  s_wszCurrIndicator = NULL;
static IME_STATE s_ImeState = IMEUI_STATE_ENGLISH;
static BOOL s_bInsertOnType = FALSE;
static BOOL s_bEnable = FALSE;
static BOOL s_bOpen = FALSE;
static BOOL s_bIgnoreOpenStatusMsg = FALSE;
static IME_TYPE s_ImeType = IME_INVALID;

// Component String information
static WCHAR   s_CompString[MAX_COMPSTRING_SIZE];			// Composition string position. Updated every frame.
static int     s_nCompCaret;								// Caret position of the composition string
static BYTE    s_abCompStringAttr[MAX_COMPSTRING_SIZE];

static HINSTANCE s_hDllImm32 = NULL;      // IMM32 DLL handle
static HINSTANCE s_hDllVer = NULL;        // Version DLL handle
static HINSTANCE s_hDllIme = NULL;


static WORD sGetLanguage() { return LOWORD( s_hklCurrent ); }
static WORD sGetPrimaryLanguage() { return PRIMARYLANGID( LOWORD( s_hklCurrent ) ); }
static WORD sGetSubLanguage() { return SUBLANGID( LOWORD( s_hklCurrent ) ); }

// Custom handlers
static void* (*sHandlerLanguageChange)(void*) = NULL;
static void* (*sHandlerReadingWindowChange)(void*) = NULL;
static void* (*sHandlerCandidateWindowChange)(void*) = NULL;

static void sResetCompositionString()
{
	s_nCompCaret = 0;
	s_CompString[0] = L'\0';
	ZeroMemory( s_abCompStringAttr, sizeof(s_abCompStringAttr) );
}

static BOOL sSetupImeApi(HKL hklCurrent)
{
	char szImeFile[DEFAULT_FILE_WITH_PATH_SIZE + 1];

	_GetReadingString = NULL;
	_ShowReadingWindow = NULL;

	s_ImeType = IME_INVALID;
	if (_ImmGetIMEFileNameA(hklCurrent, szImeFile, DEFAULT_FILE_WITH_PATH_SIZE) == 0) {
		return TRUE;
	}
	trace("%s\n", szImeFile);
	if(s_hDllIme != NULL) {
		ASSERT_RETFALSE(FreeLibrary(s_hDllIme));
	}

	for (UINT32 i = 0; i < ARRAYSIZE(listIme); i++) {
		if (PStrICmp(szImeFile, listIme[i].imeFilename) == 0) {
			s_ImeType = listIme[i].imeType;
			s_wszCurrIndicator = listIme[i].imeLabel;
			break;
		}
	}

	s_hDllIme = LoadLibraryA( szImeFile );
	ASSERT_RETFALSE(s_hDllIme != NULL);
	trace("dll file %s\n", szImeFile);
	_GetReadingString = (UINT (WINAPI*)(HIMC, UINT, LPWSTR, PINT, BOOL*, PUINT))
		( GetProcAddress( s_hDllIme, "GetReadingString" ) );
	_ShowReadingWindow =(BOOL (WINAPI*)(HIMC, BOOL))
		( GetProcAddress( s_hDllIme, "ShowReadingWindow" ) );
	return TRUE;
}

static BOOL sInitializeFunc()
{
	ASSERT_RETFALSE(s_hDllImm32 == NULL);
	ASSERT_RETFALSE(s_hDllVer == NULL);

	FARPROC Temp;

	s_hDllImm32 = LoadLibraryW( IMM32_DLLNAME );
	ASSERT(s_hDllImm32 != NULL);
	if( s_hDllImm32 )
	{
//		GETPROCADDRESS( s_hDllImm32, ImmLockIMC, Temp );
//		GETPROCADDRESS( s_hDllImm32, ImmUnlockIMC, Temp );
//		GETPROCADDRESS( s_hDllImm32, ImmLockIMCC, Temp );
//		GETPROCADDRESS( s_hDllImm32, ImmUnlockIMCC, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmCreateContext, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmDestroyContext, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmDisableTextFrameService, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetCompositionStringW, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetCandidateListW, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetContext, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmReleaseContext, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmAssociateContext, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetOpenStatus, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmSetOpenStatus, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetConversionStatus, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetDefaultIMEWnd, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetIMEFileNameA, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmGetVirtualKey, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmNotifyIME, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmSetConversionStatus, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmSimulateHotKey, Temp );
		GETPROCADDRESS( s_hDllImm32, ImmIsIME, Temp );
	}

	s_hDllVer = LoadLibraryW( VER_DLLNAME );
	ASSERT(s_hDllVer != NULL);
	if( s_hDllVer )
	{
		GETPROCADDRESS( s_hDllVer, VerQueryValueA, Temp );
		GETPROCADDRESS( s_hDllVer, GetFileVersionInfoA, Temp );
		GETPROCADDRESS( s_hDllVer, GetFileVersionInfoSizeA, Temp );
	}

	return TRUE;
}

BOOL IMM_Initialize(
	MEMORYPOOL* pPool)
{
	HIMC hIMC;

	s_bEnableImeSystem = FALSE;
//	s_hklCurrent = GetKeyboardLayout(0);
	s_listLocales = NULL;
	s_countLocales = 0;
	s_pPool = pPool;
	s_hDllIme = NULL;
	s_bEnable = FALSE;
	s_bOpen = FALSE;
	s_bIgnoreOpenStatusMsg = FALSE;

	sInitializeFunc();

	s_hIMC = _ImmCreateContext();

	hIMC = _ImmGetContext(AppCommonGetHWnd());
	if (hIMC != NULL) {
		_ImmAssociateContext(AppCommonGetHWnd(), NULL);
		_ImmReleaseContext(AppCommonGetHWnd(), hIMC);
	}

	IMM_UpdateLanguageChange();

	return TRUE;
}

BOOL IMM_Shutdown()
{
	if (s_hDllIme) {
		FreeLibrary(s_hDllIme);
		s_hDllIme = NULL;
	}
	if (s_hDllImm32) {
		FreeLibrary(s_hDllImm32);
		s_hDllImm32 = NULL;
	}
	if (s_hDllVer) {
		FreeLibrary(s_hDllVer);
		s_hDllVer = NULL;
	}
	if (s_hIMC != NULL) {
		_ImmAssociateContext(AppCommonGetHWnd(), NULL);
		_ImmDestroyContext(s_hIMC);
	}
	FREE(s_pPool, s_listLocales);
	return TRUE;
}

//--------------------------------------------------------------------------------------
//  GetImeId( UINT uIndex )
//      returns 
//  returned value:
//  0: In the following cases
//      - Non Chinese IME input locale
//      - Older Chinese IME
//      - Other error cases
//
//  Othewise:
//      When uIndex is 0 (default)
//          bit 31-24:  Major version
//          bit 23-16:  Minor version
//          bit 15-0:   Language ID
//      When uIndex is 1
//          pVerFixedInfo->dwFileVersionLS
//
//  Use IMEID_VER and IMEID_LANG macro to extract version and language information.
//  

// We define the locale-invariant ID ourselves since it doesn't exist prior to WinXP
// For more information, see the CompareString() reference.
#define LCID_INVARIANT MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)

static DWORD sGetImeId(
	MEMORYPOOL* pPool,
	UINT uIndex = 0)
{
	static HKL hklPrev = 0;
	static DWORD dwID[2] = { 0, 0 };  // Cache the result

	DWORD   dwVerSize;
	DWORD   dwVerHandle;
	LPVOID  lpVerBuffer;
	LPVOID  lpVerData;
	UINT    cbVerData;
	char    szTmp[1024];

	if( uIndex >= sizeof( dwID ) / sizeof( dwID[0] ) )
		return 0;

	if( hklPrev == s_hklCurrent )
		return dwID[uIndex];

	hklPrev = s_hklCurrent;  // Save for the next invocation

	// Check if we are using an older Chinese IME
	if( !( ( s_hklCurrent == _CHT_HKL ) || ( s_hklCurrent == _CHT_HKL2 ) || ( s_hklCurrent == _CHS_HKL ) ) )
	{
		dwID[0] = dwID[1] = 0;
		return dwID[uIndex];
	}

	// Obtain the IME file name
	if ( !_ImmGetIMEFileNameA( s_hklCurrent, szTmp, ( sizeof(szTmp) / sizeof(szTmp[0]) ) - 1 ) )
	{
		dwID[0] = dwID[1] = 0;
		return dwID[uIndex];
	}

	// Check for IME that doesn't implement reading string API
	if ( !_GetReadingString )
	{
		if( ( CompareStringA( LCID_INVARIANT, NORM_IGNORECASE, szTmp, -1, CHT_IMEFILENAME1, -1 ) != CSTR_EQUAL ) &&
			( CompareStringA( LCID_INVARIANT, NORM_IGNORECASE, szTmp, -1, CHT_IMEFILENAME2, -1 ) != CSTR_EQUAL ) &&
			( CompareStringA( LCID_INVARIANT, NORM_IGNORECASE, szTmp, -1, CHT_IMEFILENAME3, -1 ) != CSTR_EQUAL ) &&
			( CompareStringA( LCID_INVARIANT, NORM_IGNORECASE, szTmp, -1, CHS_IMEFILENAME1, -1 ) != CSTR_EQUAL ) &&
			( CompareStringA( LCID_INVARIANT, NORM_IGNORECASE, szTmp, -1, CHS_IMEFILENAME2, -1 ) != CSTR_EQUAL ) )
		{
			dwID[0] = dwID[1] = 0;
			return dwID[uIndex];
		}
	}

	dwVerSize = _GetFileVersionInfoSizeA( szTmp, &dwVerHandle );
	if (dwVerSize == 0) {
		return dwID[uIndex];
	}

	lpVerBuffer = MALLOC(pPool, dwVerSize);
	if (lpVerBuffer == NULL) {
		return dwID[uIndex];
	}

	if(_GetFileVersionInfoA(szTmp, dwVerHandle, dwVerSize, lpVerBuffer)) {
		if(_VerQueryValueA(lpVerBuffer, "\\", &lpVerData, &cbVerData)) {
			DWORD dwVer = ( (VS_FIXEDFILEINFO*)lpVerData )->dwFileVersionMS;
			dwVer = ( dwVer & 0x00ff0000 ) << 8 | ( dwVer & 0x000000ff ) << 16;
			if(_GetReadingString ||
				(sGetLanguage() == LANG_CHT &&
				(	dwVer == MAKEIMEVERSION(4, 2) || 
				dwVer == MAKEIMEVERSION(4, 3) || 
				dwVer == MAKEIMEVERSION(4, 4) || 
				dwVer == MAKEIMEVERSION(5, 0) ||
				dwVer == MAKEIMEVERSION(5, 1) ||
				dwVer == MAKEIMEVERSION(5, 2) ||
				dwVer == MAKEIMEVERSION(6, 0))) ||
				(sGetLanguage() == LANG_CHS &&
				(	dwVer == MAKEIMEVERSION(4, 1) ||
				dwVer == MAKEIMEVERSION(4, 2) ||
				dwVer == MAKEIMEVERSION(5, 3)))) {
					dwID[0] = dwVer | sGetLanguage();
					dwID[1] = ( (VS_FIXEDFILEINFO*)lpVerData )->dwFileVersionLS;
			}
		}
	}
	FREE(pPool, lpVerBuffer);

	return dwID[uIndex];
}

static void sCheckInputLocale()
{
	HKL hklNew = GetKeyboardLayout( 0 );
	if (s_hklCurrent == hklNew) {
		return;
	} else {
		s_hklCurrent = hklNew;
	}

	switch (sGetPrimaryLanguage())
	{
		// Chinese
		case LANG_CHINESE:
			s_bVerticalCand = TRUE;
			switch ( sGetSubLanguage() )
			{
				case SUBLANG_CHINESE_SIMPLIFIED:
					s_wszCurrIndicator = s_aszLanguage[INDICATOR_CHS];
					s_bVerticalCand = (sGetImeId(NULL) == 0);
					break;
				case SUBLANG_CHINESE_TRADITIONAL:
					s_wszCurrIndicator = s_aszLanguage[INDICATOR_CHT];
					break;
				default:    // unsupported sub-language
					s_wszCurrIndicator = s_aszLanguage[INDICATOR_NON_IME];
					break;
			}
			break;

		// Korean
		case LANG_KOREAN:
			s_wszCurrIndicator = s_aszLanguage[INDICATOR_KOREAN];
			s_bVerticalCand = false;
			break;

		// Japanese
		case LANG_JAPANESE:
			s_wszCurrIndicator = s_aszLanguage[INDICATOR_JAPANESE];
			s_bVerticalCand = true;
			break;

		default:
			// A non-IME language.  Obtain the language abbreviation
			// and store it for rendering the indicator later.
			s_wszCurrIndicator = s_aszLanguage[INDICATOR_NON_IME];
	}

	// If non-IME, use the language abbreviation.
	if( s_wszCurrIndicator == s_aszLanguage[INDICATOR_NON_IME] )
	{
		WCHAR wszLang[5];
		GetLocaleInfoW( MAKELCID( LOWORD( s_hklCurrent ), SORT_DEFAULT ), LOCALE_SABBREVLANGNAME, wszLang, 5 );
		s_wszCurrIndicator[0] = wszLang[0];
		s_wszCurrIndicator[1] = towlower( wszLang[1] );
	}
}


//--------------------------------------------------------------------------------------
static void sCheckToggleState()
{
	sCheckInputLocale();

	CAutoIMContext autoIMC(AppCommonGetHWnd());
	HIMC hImc = autoIMC.getContextHandle();
	if (hImc == NULL) {
		s_ImeState = IMEUI_STATE_OFF;
		return;
	}

	BOOL bIme = _ImmIsIME(s_hklCurrent);
	BOOL bChineseIME = bIme && (sGetPrimaryLanguage() == LANG_CHINESE);
	if (bChineseIME) {
		DWORD dwConvMode, dwSentMode;
		_ImmGetConversionStatus( hImc, &dwConvMode, &dwSentMode );
		s_ImeState = (dwConvMode & IME_CMODE_NATIVE) ? IMEUI_STATE_ON : IMEUI_STATE_ENGLISH;
	} else {
		s_ImeState = (bIme && _ImmGetOpenStatus(hImc)) ? IMEUI_STATE_ON : IMEUI_STATE_OFF;
	}
}

BOOL IMM_UpdateLanguageChange()
{
	sCheckToggleState();

	// Korean IME always inserts on keystroke.  Other IMEs do not.
	s_bInsertOnType = (sGetPrimaryLanguage() == LANG_KOREAN);

	// IME changed.  Setup the new IME.
	sSetupImeApi(s_hklCurrent);
	if (_ShowReadingWindow)
	{
		CAutoIMContext autoIMC(AppCommonGetHWnd());
		HIMC hImc = autoIMC.getContextHandle();
		if (hImc != NULL) {
			_ShowReadingWindow(hImc, FALSE);
		}
	}
	UIRepaint();
	return TRUE;
}

BOOL IMM_StartComposition()
{
	sResetCompositionString();
	return TRUE;
}

BOOL IMM_EndComposition()
{
	sResetCompositionString();
	return TRUE;
}

LRESULT CALLBACK c_PrimeWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam);

static void sSendCompositeString(BOOL bDeleteLast, LPCWSTR str)
{
	if (bDeleteLast) {
		SendMessage(AppCommonGetHWnd(), WM_UNICHAR, L'\b', 0);
	}
	for (UINT32 i = 0; str[i] != L'\0'; i++) {
		SendMessage(AppCommonGetHWnd(), WM_UNICHAR, (WPARAM)str[i], 0);
	}
}

BOOL IMM_ProcessComposition(LPARAM lParam)
{
	WCHAR wszCompStr[MAX_COMPSTRING_SIZE+1];
	static BOOL bDeleteLast = FALSE;
	CAutoIMContext autoIMC(AppCommonGetHWnd());
	HIMC hImc = autoIMC.getContextHandle();
	if (hImc == NULL) {
		return TRUE;
	}

	LONG pos = -1;
	if (lParam & GCS_CURSORPOS) {
		pos = _ImmGetCompositionStringW(hImc, GCS_CURSORPOS, NULL, 0);
		pos &= 0xFFFF;
	}

	if (lParam & GCS_RESULTSTR) {
//		trace("  result str\n");
		UINT32 len;
		len = _ImmGetCompositionStringW(hImc, GCS_RESULTSTR, wszCompStr, MAX_COMPSTRING_SIZE * sizeof(WCHAR));
		if (len > 0) {
			len /= sizeof(WCHAR);
			wszCompStr[len] = L'\0';
			sSendCompositeString(bDeleteLast, wszCompStr);
			sResetCompositionString();
			bDeleteLast = FALSE;
		}
	}

	if (lParam & GCS_COMPSTR) {
//		trace("  comp str\n");
		LONG len;
		len = _ImmGetCompositionStringW(hImc, GCS_COMPSTR, wszCompStr, MAX_COMPSTRING_SIZE * sizeof(WCHAR));
		if (len > 0) {
			len /= sizeof(WCHAR);
			wszCompStr[len] = L'\0';
			if (s_bInsertOnType) {
				sSendCompositeString(bDeleteLast, wszCompStr);
				for (UINT32 i = 0; wszCompStr[i] != '\0'; i++) {
					SendMessage( AppCommonGetHWnd(), WM_KEYDOWN, VK_LEFT, 0 );
				}
				SendMessage( AppCommonGetHWnd(), WM_KEYUP, VK_LEFT, 0 );
				bDeleteLast = TRUE;
			} else {
				if (sHandlerReadingWindowChange) {
					if (pos >= 0 && pos < len) {
						for (LONG i = len; i >= pos; i--) {
							wszCompStr[i+1] = wszCompStr[i];
						}
						wszCompStr[pos] = L' ';
					}
					sHandlerReadingWindowChange(wszCompStr);
				}
			}
		} else {
			if (s_bInsertOnType) {
				sSendCompositeString(bDeleteLast, L"");
				bDeleteLast = FALSE;
			}
			if (!s_bInsertOnType && sHandlerReadingWindowChange != NULL) {
				sHandlerReadingWindowChange(NULL);
			}
		}
	} else {
		if (!s_bInsertOnType && sHandlerReadingWindowChange != NULL) {
			sHandlerReadingWindowChange(NULL);
		}
	}
	return TRUE;
}

BOOL IMM_Enable(
	BOOL bEnable)
{
/*	CAutoIMContext autoIMC(AppCommonGetHWnd());
	HIMC hImc = autoIMC.getContextHandle();
	if (hImc == NULL) {
		return TRUE;
	}
	//	ASSERT_RETFALSE(hImc != NULL);

	if (bEnable == s_bEnable) {
		return TRUE;
	}

	s_bEnable = bEnable;
	BOOL bOpen = _ImmGetOpenStatus(hImc);
	if (s_bOpen) {
		s_bIgnoreOpenStatusMsg = TRUE;
		if (sGetPrimaryLanguage() == LANG_KOREAN) {
			ASSERT_RETFALSE(_ImmSimulateHotKey(AppCommonGetHWnd(), IME_KHOTKEY_SHAPE_TOGGLE));
//			ASSERT_RETFALSE(_ImmSetOpenStatus(hImc, bEnable));
		}
	}
	
	ASSERT_RETFALSE(_ImmSetOpenStatus(hImc, bEnable));*/
	if (bEnable) {
		_ImmAssociateContext(AppCommonGetHWnd(), s_hIMC);
	} else {
		_ImmAssociateContext(AppCommonGetHWnd(), NULL);
	}

	s_bEnable = bEnable;
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_IME_LANG_INDICATOR);
	if (pComponent)
		UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
	
	return TRUE;
}

BOOL IMM_UpdateOpenStatus()
{
	CAutoIMContext autoIMC(AppCommonGetHWnd());
	HIMC hImc = autoIMC.getContextHandle();
	if (hImc == NULL) {
		return TRUE;
	}
	//	ASSERT_RETFALSE(hImc != NULL);

	if (_ImmGetOpenStatus(hImc)) {
		//		ASSERT_RETFALSE(_ImmSetOpenStatus(hImc, FALSE));
		trace("IMMNotify open status on\n");
	} else {
		trace("IMMNotify open status off\n");
	}
	return TRUE;

/*	BOOL bOpen = _ImmGetOpenStatus(hImc);
	if (s_bIgnoreOpenStatusMsg) {
		s_bIgnoreOpenStatusMsg = FALSE;
		ASSERT_RETFALSE(s_bOpen == bOpen);
		return TRUE;
	}

	s_bOpen = bOpen;
	if (bOpen && !s_bEnable) {
		s_bIgnoreOpenStatusMsg = TRUE;
		ASSERT_RETFALSE(_ImmSetOpenStatus(hImc, FALSE));
	}
	return TRUE;*/
}

BOOL IMM_IgnoreOpenStatus()
{
	CAutoIMContext autoIMC(AppCommonGetHWnd());
	HIMC hImc = autoIMC.getContextHandle();
	if (hImc == NULL) {
		return TRUE;
	}
	//	ASSERT_RETFALSE(hImc != NULL);

	if (_ImmGetOpenStatus(hImc)) {
//		ASSERT_RETFALSE(_ImmSetOpenStatus(hImc, FALSE));
		trace("IMMNotify open status on\n");
	} else {
		trace("IMMNotify open status off\n");
	}
	return TRUE;
}

DWORD IMM_GetVirtualKey()
{
	return _ImmGetVirtualKey(AppCommonGetHWnd());;
}

extern LONG iIgnoreEscapeKeyUpMsg;  //Used for temp hack to ignore escape key-up messages

BOOL IMM_HandleNotify(
	WPARAM wParam,
	LPARAM lParam,
	BOOL* bTrapped)
{
	ASSERT_RETFALSE(bTrapped != NULL);

	*bTrapped = FALSE;
	switch (wParam) {
		case IMN_SETOPENSTATUS:
			sCheckToggleState();
			UIRepaint();			// to change the language indicator
			break;

		case IMN_PRIVATE:
			// don't draw the reading window at all for now.
			*bTrapped = TRUE;
			break; 

		case IMN_OPENCANDIDATE:
			if (sGetPrimaryLanguage() == LANG_KOREAN) {
				CAutoIMContext autoIMC(AppCommonGetHWnd());
				HIMC hImc = autoIMC.getContextHandle();
				if (hImc != NULL) {
					_ImmNotifyIME(hImc, NI_CLOSECANDIDATE, 0, 0);
				}
				*bTrapped = TRUE;
				break;
			}

		case IMN_CHANGECANDIDATE:
			if (sHandlerCandidateWindowChange != NULL)
			{
				CAutoIMContext autoIMC(AppCommonGetHWnd());
				HIMC hImc = autoIMC.getContextHandle();
				ASSERT_RETFALSE(hImc != NULL);

				WCHAR* strCandidates[MAX_CANDIDATE_COUNT+2];
				WCHAR strCandidateBuf[MAX_CANDIDATE_COUNT][MAX_CANDIDATE_SIZE];
				UINT32 i, j;

				// CML 2007.09.30 - This is to work around a warning (C4334) on x64.
				LPARAM lp1 = (LPARAM)1;

				for (UINT32 index = 0; index < sizeof(LPARAM)*8; index++) {
					if (lParam & (lp1 << index)) {
						DWORD bufSize = _ImmGetCandidateListW(hImc, index, NULL, 0);
						ASSERT_CONTINUE(bufSize != 0);

						CANDIDATELIST* pCandidateList = (CANDIDATELIST*)MALLOC(NULL, bufSize);
						ASSERT_CONTINUE(pCandidateList != NULL);
						
						ASSERT_GOTO(_ImmGetCandidateListW(hImc, index, pCandidateList, bufSize) != 0, _err);

						ASSERT_GOTO(pCandidateList->dwPageSize != 0, _err);
						ASSERT_GOTO(pCandidateList->dwPageSize <= MAX_CANDIDATE_COUNT, _err);

						for (i = 0, j = pCandidateList->dwPageStart;
							i < pCandidateList->dwPageSize && j < pCandidateList->dwCount;
							i++, j++) {
							if (i < 9) {
								PStrPrintf(strCandidateBuf[i], MAX_CANDIDATE_SIZE, L"  %d %s", i+1,
									(WCHAR*)((BYTE*)pCandidateList + pCandidateList->dwOffset[j]));
							} else {
								PStrPrintf(strCandidateBuf[i], MAX_CANDIDATE_SIZE, L" %d %s", i+1,
									(WCHAR*)((BYTE*)pCandidateList + pCandidateList->dwOffset[j]));
							}
						}
						if (s_ImeType != IME_CHT_PHONETIC) {
							strCandidateBuf[pCandidateList->dwSelection - pCandidateList->dwPageStart][0] = L'*';
						}
						for (j = 0; j < i; j++) {
							strCandidates[j] = strCandidateBuf[j];
						}
						strCandidates[i] = NULL;

						sHandlerCandidateWindowChange(strCandidates);
_err:
						FREE(NULL, pCandidateList);
					}
				}
			}
			*bTrapped = TRUE;
			break;

		case IMN_CLOSECANDIDATE:
			if (sGetPrimaryLanguage() == LANG_KOREAN) {
				*bTrapped = TRUE;
				break;
			}

			if (sHandlerCandidateWindowChange != NULL) {
				sHandlerCandidateWindowChange(NULL);
			}
			// Ignore the next escape key up message
			InterlockedExchange(&iIgnoreEscapeKeyUpMsg, 1);
			*bTrapped = TRUE;
			break;

		// doesn't support anything else yet
		default:
			trace("Notify - %d  0x%llx\n", wParam, lParam);
			break;
	}
	return TRUE;
}

void IMM_GetLangIndicator(
	LPWSTR buf,
	DWORD len)
{
	UINT32 i = INDICATOR_NON_IME;
	
	switch (sGetPrimaryLanguage())
	{
		// Chinese
		case LANG_CHINESE:
			switch ( sGetSubLanguage() )
			{
			case SUBLANG_CHINESE_SIMPLIFIED:
				i = INDICATOR_CHS;
				break;
			case SUBLANG_CHINESE_TRADITIONAL:
				i = INDICATOR_CHT;
				break;
			default:    // unsupported sub-language
				i = INDICATOR_NON_IME;
				break;
			}
			break;

		// Korean
		case LANG_KOREAN:
			i = INDICATOR_KOREAN;
			break;

		// Japanese
		case LANG_JAPANESE:
			i = INDICATOR_JAPANESE;
			break;

		default:
			// A non-IME language.  Obtain the language abbreviation
			// and store it for rendering the indicator later.
			i = INDICATOR_NON_IME;
			break;
	}
	if (s_ImeType == IME_INVALID) {
		PStrPrintf(buf, len, L"%s %c", s_aszLanguage[i], s_aszIndicator[(s_ImeState == IMEUI_STATE_ON) ? i : INDICATOR_NON_IME]);
	} else {
		PStrPrintf(buf, len, L"%s %s", s_aszLanguage[i], s_wszCurrIndicator);
	}
//	PStrCopy(buf, s_aszLanguage[i], len);
}

DWORD IMM_GetPrimaryLanguage()
{
	return sGetPrimaryLanguage();
}

BOOL IMM_IsEnabled()
{
	return s_bEnable;
}

void IMM_SetLanguageChangeHandler(void* (*handler)(void*))
{
	sHandlerLanguageChange = handler;
}

void IMM_SetReadingWindowChangeHandler(void* (*handler)(void*))
{
	sHandlerReadingWindowChange = handler;
}

void IMM_SetCandidateWindowChangeHandler(void* (*handler)(void*))
{
	sHandlerCandidateWindowChange = handler;
}


#endif
